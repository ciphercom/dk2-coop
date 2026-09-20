#include "pipe.h"

#include <sddl.h>
#include <chrono>
#include <cstdlib>
#include <stdexcept>
#include <vector>

namespace patch::diagnostic {
namespace {
constexpr DWORD ioTimeout = 1500;
constexpr auto gameTimeout = std::chrono::milliseconds(1000);

/** Restrict the endpoint to this process's Windows account and SYSTEM. */
PSECURITY_DESCRIPTOR localAccountSecurity() {
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) throw std::runtime_error("diagnostic token query failed");
    DWORD size = 0;
    GetTokenInformation(token, TokenUser, nullptr, 0, &size);
    std::vector<char> data(size);
    const BOOL read = GetTokenInformation(token, TokenUser, data.data(), size, &size);
    CloseHandle(token);
    if (!read) throw std::runtime_error("diagnostic account query failed");
    LPSTR sid = nullptr;
    if (!ConvertSidToStringSidA(reinterpret_cast<TOKEN_USER *>(data.data())->User.Sid, &sid))
        throw std::runtime_error("diagnostic account SID failed");
    const std::string dacl = std::string("D:P(A;;GA;;;SY)(A;;GA;;;") + sid + ")";
    LocalFree(sid);
    PSECURITY_DESCRIPTOR descriptor = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorA(dacl.c_str(), SDDL_REVISION_1, &descriptor, nullptr))
        throw std::runtime_error("diagnostic pipe security failed");
    return descriptor;
}
}

Pipe::Pipe() : gameThread_(GetCurrentThreadId()) {
    auto descriptor = localAccountSecurity();
    SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES), descriptor, FALSE};
    const std::string name = "\\\\.\\pipe\\flame-diagnostic-" + std::to_string(GetCurrentProcessId());
    pipe_ = CreateNamedPipeA(name.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
        1, static_cast<DWORD>(responseLimit), static_cast<DWORD>(requestLimit), 0, &security);
    LocalFree(descriptor);
    stop_ = CreateEventA(nullptr, TRUE, FALSE, nullptr);
    ioEvent_ = CreateEventA(nullptr, TRUE, FALSE, nullptr);
    if (pipe_ == INVALID_HANDLE_VALUE || !stop_ || !ioEvent_) {
        if (pipe_ != INVALID_HANDLE_VALUE) CloseHandle(pipe_);
        if (stop_) CloseHandle(stop_);
        if (ioEvent_) CloseHandle(ioEvent_);
        throw std::runtime_error("diagnostic pipe creation failed");
    }
    try { worker_ = std::thread([this] { serve(); }); }
    catch (...) {
        CloseHandle(pipe_); CloseHandle(stop_); CloseHandle(ioEvent_);
        throw;
    }
}

/** Signal every worker wait before joining; never run this from DllMain. */
Pipe::~Pipe() {
    SetEvent(stop_);
    ready_.notify_all();
    worker_.join();
    CloseHandle(pipe_);
    CloseHandle(stop_);
    CloseHandle(ioEvent_);
}

/** Cancellation is reaped before the stack-owned OVERLAPPED goes out of scope. */
bool Pipe::complete(BOOL immediate, OVERLAPPED &operation, DWORD &bytes, DWORD timeout) {
    if (immediate) return true;
    if (GetLastError() != ERROR_IO_PENDING) return false;
    const HANDLE waits[] = {stop_, ioEvent_};
    if (WaitForMultipleObjects(2, waits, FALSE, timeout) == WAIT_OBJECT_0 + 1)
        return GetOverlappedResult(pipe_, &operation, &bytes, FALSE) != FALSE;
    CancelIoEx(pipe_, &operation);
    GetOverlappedResult(pipe_, &operation, &bytes, TRUE);
    return false;
}

/** The game thread never waits for the transport; an occupied mailbox is retried next tick. */
void Pipe::pump(Core &core, const SnapshotProvider &snapshot, const LoggingToggle &logging) {
    if (GetCurrentThreadId() != gameThread_) std::abort();
    std::unique_lock lock(mutex_, std::try_to_lock);
    if (!lock || !pending_) return;
    response_ = core.execute(request_, snapshot, logging);
    if (response_.size() > responseLimit) std::abort();
    pending_ = false;
    ready_.notify_one();
}

/** Only a completed request enters the single mailbox; timed-out work is removed. */
std::string Pipe::dispatch(const std::string &request) {
    std::unique_lock lock(mutex_);
    request_ = request;
    response_.clear();
    pending_ = true;
    ready_.wait_for(lock, gameTimeout, [this] { return !pending_ || WaitForSingleObject(stop_, 0) == WAIT_OBJECT_0; });
    if (pending_) {
        pending_ = false;
        return error("game_thread_timeout");
    }
    return response_;
}

/** Bounded reads and writes prevent idle, disconnected or slow clients from pinning shutdown. */
void Pipe::serve() {
    while (WaitForSingleObject(stop_, 0) != WAIT_OBJECT_0) {
        ResetEvent(ioEvent_);
        OVERLAPPED operation{};
        operation.hEvent = ioEvent_;
        DWORD bytes = 0;
        BOOL connected = ConnectNamedPipe(pipe_, &operation);
        if (!connected && GetLastError() == ERROR_PIPE_CONNECTED) connected = TRUE;
        if (!complete(connected, operation, bytes, INFINITE)) {
            // A client can close between CreateFile and ConnectNamedPipe (ERROR_NO_DATA).
            // Reset that instance before listening again or every later connection stays busy.
            DisconnectNamedPipe(pipe_);
            continue;
        }
        std::string request, response;
        const ULONGLONG deadline = GetTickCount64() + ioTimeout;
        while (response.empty()) {
            char buffer[requestLimit];
            ResetEvent(ioEvent_);
            operation = {};
            operation.hEvent = ioEvent_;
            bytes = 0;
            const auto now = GetTickCount64();
            if (now >= deadline) break;
            const BOOL read = ReadFile(pipe_, buffer, sizeof(buffer), &bytes, &operation);
            if (!complete(read, operation, bytes, static_cast<DWORD>(deadline - now)) || bytes == 0) break;
            request.append(buffer, bytes);
            if (request.size() > requestLimit) { response = error("request_too_large"); break; }
            const auto newline = request.find('\n');
            if (newline != std::string::npos) {
                if (newline != request.size() - 1) response = error("unknown_command");
                else response = dispatch(request.substr(0, newline));
                break;
            }
            if (request.size() == requestLimit) { response = error("request_too_large"); break; }
        }
        if (!response.empty() && WaitForSingleObject(stop_, 0) != WAIT_OBJECT_0) {
            ResetEvent(ioEvent_);
            operation = {};
            operation.hEvent = ioEvent_;
            bytes = 0;
            complete(WriteFile(pipe_, response.data(), static_cast<DWORD>(response.size()), &bytes, &operation),
                operation, bytes, ioTimeout);
            // A buffered write can finish before the client reads it. Let the client close
            // after reading; immediate DisconnectNamedPipe would discard its unread reply.
            const ULONGLONG closeDeadline = GetTickCount64() + ioTimeout;
            while (WaitForSingleObject(stop_, 0) != WAIT_OBJECT_0) {
                const auto now = GetTickCount64();
                if (now >= closeDeadline) break;
                ResetEvent(ioEvent_);
                operation = {};
                operation.hEvent = ioEvent_;
                char ignored[requestLimit];
                bytes = 0;
                const BOOL read = ReadFile(pipe_, ignored, sizeof(ignored), &bytes, &operation);
                if (!complete(read, operation, bytes, static_cast<DWORD>(closeDeadline - now)) || bytes == 0) break;
                // An oversized request may still have buffered bytes. Consume them until
                // client EOF, rather than mistaking trailing input for a read acknowledgement.
            }
        }
        // FlushFileBuffers waits for a client read and would make shutdown unbounded.
        DisconnectNamedPipe(pipe_);
    }
}
}
