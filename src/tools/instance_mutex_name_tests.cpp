#include "instance_mutex_name.h"

#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

void requireInvalidPathRejected() {
    try {
        patch::instance_mutex::nameForExecutablePath(L"DKII-DX.exe");
    } catch (const std::invalid_argument &) {
        return;
    }
    throw std::runtime_error("a path without an installation directory must be rejected");
}

}

/** Verify that mutex identity follows the installation directory, not path spelling or file name. */
int main() {
    const std::string server = patch::instance_mutex::nameForExecutablePath(
        L"C:\\Games\\Dungeon Keeper 2\\DKII-DX.exe"
    );
    const std::string equivalentServer = patch::instance_mutex::nameForExecutablePath(
        L"c:/games/dungeon keeper 2/DKII.exe"
    );
    const std::string client = patch::instance_mutex::nameForExecutablePath(
        L"C:\\Games\\Dungeon Keeper 2 Client\\DKII-DX.exe"
    );
    const std::string sameFolderNameOnAnotherDrive = patch::instance_mutex::nameForExecutablePath(
        L"D:\\Games\\Dungeon Keeper 2\\DKII-DX.exe"
    );

    require(server.starts_with("DKII MUTEX "), "mutex name must retain the DKII prefix");
    require(server == equivalentServer, "equivalent installation paths must share a mutex");
    require(server != client, "different installation folders must use different mutexes");
    require(server != sameFolderNameOnAnotherDrive, "the full installation path must identify the mutex");
    requireInvalidPathRejected();
    return 0;
}
