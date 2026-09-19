#include "instance_mutex_name.h"

#include <Windows.h>

#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <utility>

namespace {

/** Normalize a Windows installation directory before deriving its stable identity. */
std::wstring normalizedDirectory(std::wstring executablePath) {
    const size_t separator = executablePath.find_last_of(L"\\/");
    if (separator == std::wstring::npos || separator == 0) {
        throw std::invalid_argument("executable path does not contain a directory");
    }
    executablePath.resize(separator);
    for (wchar_t &character : executablePath) {
        if (character == L'/') character = L'\\';
    }
    if (!executablePath.empty() && !CharUpperBuffW(executablePath.data(), static_cast<DWORD>(executablePath.size()))) {
        throw std::runtime_error("failed to normalize executable directory");
    }
    return executablePath;
}

/** Hash UTF-16 code units explicitly so the mutex suffix is stable across builds. */
uint64_t fnv1a(const std::wstring &value) {
    uint64_t hash = 14695981039346656037ULL;
    for (const wchar_t character : value) {
        const uint16_t codeUnit = static_cast<uint16_t>(character);
        hash ^= codeUnit & 0xFF;
        hash *= 1099511628211ULL;
        hash ^= codeUnit >> 8;
        hash *= 1099511628211ULL;
    }
    return hash;
}

}

std::string patch::instance_mutex::nameForExecutablePath(std::wstring executablePath) {
    const uint64_t installationHash = fnv1a(normalizedDirectory(std::move(executablePath)));
    char mutexName[32];
    const int length = std::snprintf(
        mutexName,
        sizeof(mutexName),
        "DKII MUTEX %016llX",
        static_cast<unsigned long long>(installationHash)
    );
    if (length < 0 || static_cast<size_t>(length) >= sizeof(mutexName)) {
        throw std::runtime_error("failed to format installation mutex name");
    }
    return mutexName;
}

std::string patch::instance_mutex::currentName() {
    std::wstring executablePath(MAX_PATH, L'\0');
    while (true) {
        const DWORD length = GetModuleFileNameW(
            nullptr,
            executablePath.data(),
            static_cast<DWORD>(executablePath.size())
        );
        if (length == 0) throw std::runtime_error("failed to resolve executable path");
        if (length < executablePath.size()) {
            executablePath.resize(length);
            return nameForExecutablePath(std::move(executablePath));
        }
        if (executablePath.size() >= 32768) {
            throw std::runtime_error("executable path exceeds the Windows path limit");
        }
        executablePath.resize(executablePath.size() * 2);
    }
}
