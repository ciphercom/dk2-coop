#pragma once

#include "bridge.h"
#include <Windows.h>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace patch::diagnostic {
/** One local client at a time, one bounded mailbox, and cancelable worker-only pipe I/O. */
class Pipe {
public:
    Pipe();
    ~Pipe();
    Pipe(const Pipe &) = delete;
    Pipe &operator=(const Pipe &) = delete;
    void pump(Core &core, const SnapshotProvider &snapshot, const LoggingToggle &logging);
private:
    void serve();
    bool complete(BOOL immediate, OVERLAPPED &operation, DWORD &bytes, DWORD timeout);
    std::string dispatch(const std::string &request);
    HANDLE pipe_ = INVALID_HANDLE_VALUE, stop_ = nullptr, ioEvent_ = nullptr;
    std::thread worker_;
    std::mutex mutex_;
    std::condition_variable ready_;
    std::string request_, response_;
    bool pending_ = false;
    DWORD gameThread_ = 0;
};
}
