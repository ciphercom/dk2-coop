#include "pipe.h"
#include <iostream>
#include <string_view>

/** Real transport with synthetic state; stdin EOF owns the host's graceful lifetime. */
int main(int argc, char **argv) {
    using namespace patch::diagnostic;
    const bool noPump = argc == 2 && std::string_view(argv[1]) == "--no-pump";
    Pipe pipe;
    Core core;
    bool debugLogging = false;
    std::cout << GetCurrentProcessId() << std::endl;
    unsigned tick = 0;
    while (true) {
        DWORD available = 0;
        if (!PeekNamedPipe(GetStdHandle(STD_INPUT_HANDLE), nullptr, 0, nullptr, &available, nullptr)) break;
        if (!noPump) {
            pipe.pump(core, [&](Snapshot) {
                return std::string("{\"fixture\":true,\"world_active\":false,\"debug_logging\":") +
                    (debugLogging ? "true}" : "false}");
            }, [&](bool enabled) { debugLogging = enabled; });
            core.record({Boundary::local_queue_before_handle, static_cast<int>(tick), 4, 2, 11, 22, 33});
            core.record({Boundary::world_tick_return, static_cast<int>(tick++), 4, 2, 11, 22, 33});
        }
        Sleep(10);
    }
    return 0;
}
