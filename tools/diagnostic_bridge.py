"""Query one local Flame process through its opt-in, bounded Diagnostic bridge."""

import argparse
import ctypes
from ctypes import wintypes
import json
import sys
import time


class BridgeError(Exception):
    """A bounded diagnostic request failed without changing game state."""


def request(pid, command):
    """Exchange one allow-listed request; even a stalled peer has a five-second limit."""
    if sys.platform != "win32":
        raise BridgeError("windows_required")
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)

    class Overlapped(ctypes.Structure):
        _fields_ = [("Internal", ctypes.c_size_t), ("InternalHigh", ctypes.c_size_t),
                    ("Offset", wintypes.DWORD), ("OffsetHigh", wintypes.DWORD),
                    ("hEvent", wintypes.HANDLE)]

    kernel.CreateFileW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD,
                                  ctypes.c_void_p, wintypes.DWORD, wintypes.DWORD,
                                  wintypes.HANDLE]
    kernel.CreateFileW.restype = wintypes.HANDLE
    kernel.WaitNamedPipeW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD]
    kernel.WaitNamedPipeW.restype = wintypes.BOOL
    kernel.CreateEventW.argtypes = [ctypes.c_void_p, wintypes.BOOL, wintypes.BOOL,
                                   wintypes.LPCWSTR]
    kernel.CreateEventW.restype = wintypes.HANDLE
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    kernel.CloseHandle.restype = wintypes.BOOL
    kernel.WaitForSingleObject.argtypes = [wintypes.HANDLE, wintypes.DWORD]
    kernel.WaitForSingleObject.restype = wintypes.DWORD
    for operation in (kernel.ReadFile, kernel.WriteFile):
        operation.argtypes = [wintypes.HANDLE, ctypes.c_void_p, wintypes.DWORD,
                              ctypes.POINTER(wintypes.DWORD), ctypes.POINTER(Overlapped)]
        operation.restype = wintypes.BOOL
    kernel.GetOverlappedResult.argtypes = [wintypes.HANDLE, ctypes.POINTER(Overlapped),
                                          ctypes.POINTER(wintypes.DWORD), wintypes.BOOL]
    kernel.GetOverlappedResult.restype = wintypes.BOOL
    kernel.CancelIoEx.argtypes = [wintypes.HANDLE, ctypes.POINTER(Overlapped)]
    kernel.CancelIoEx.restype = wintypes.BOOL

    deadline = time.monotonic() + 5

    def remaining_ms():
        remaining = int((deadline - time.monotonic()) * 1000)
        if remaining <= 0:
            raise BridgeError("bridge_timeout")
        return remaining

    pipe_name = rf"\\.\pipe\flame-diagnostic-{pid}"
    invalid_handle = ctypes.c_void_p(-1).value
    while True:
        # Overlapped I/O makes the deadline apply to reads too, not just connection.
        pipe = kernel.CreateFileW(pipe_name, 0xC0000000, 0, None, 3, 0x40000000, None)
        if pipe != invalid_handle:
            break
        error = ctypes.get_last_error()
        if error == 5:
            raise BridgeError("bridge_access_denied")
        if error != 231:  # ERROR_PIPE_BUSY: another bounded request is being served.
            raise BridgeError("bridge_unavailable")
        if not kernel.WaitNamedPipeW(pipe_name, remaining_ms()):
            raise BridgeError("bridge_unavailable")

    def transfer(operation, buffer, size):
        event = kernel.CreateEventW(None, True, False, None)
        if not event:
            raise BridgeError("bridge_io_error")
        overlapped = Overlapped(hEvent=event)
        transferred = wintypes.DWORD()
        pending = False
        try:
            timeout = remaining_ms()
            if not operation(pipe, buffer, size, ctypes.byref(transferred), ctypes.byref(overlapped)):
                if ctypes.get_last_error() != 997:  # ERROR_IO_PENDING
                    raise BridgeError("bridge_disconnected")
                pending = True
                wait = kernel.WaitForSingleObject(event, timeout)
                if wait != 0:
                    raise BridgeError("bridge_timeout" if wait == 258 else "bridge_io_error")
            if not kernel.GetOverlappedResult(pipe, ctypes.byref(overlapped),
                                             ctypes.byref(transferred), False):
                raise BridgeError("bridge_disconnected")
            pending = False
            return transferred.value
        finally:
            if pending:
                # Keep the request's memory alive until the kernel finishes cancellation.
                kernel.CancelIoEx(pipe, ctypes.byref(overlapped))
                kernel.GetOverlappedResult(pipe, ctypes.byref(overlapped),
                                           ctypes.byref(transferred), True)
            kernel.CloseHandle(event)

    try:
        payload = (command + "\n").encode("ascii")
        outgoing = ctypes.create_string_buffer(payload)
        if transfer(kernel.WriteFile, outgoing, len(payload)) != len(payload):
            raise BridgeError("bridge_disconnected")
        response = bytearray()
        while b"\n" not in response:
            incoming = ctypes.create_string_buffer(4096)
            count = transfer(kernel.ReadFile, incoming, len(incoming))
            if not count:
                raise BridgeError("bridge_disconnected")
            response.extend(incoming.raw[:count])
            if len(response) > 65536:
                raise BridgeError("response_too_large")
        try:
            value = json.loads(response.decode("utf-8"))
        except (ValueError, UnicodeError):
            raise BridgeError("invalid_response") from None
        if not isinstance(value, dict) or type(value.get("ok")) is not bool:
            raise BridgeError("invalid_response")
        return value
    finally:
        kernel.CloseHandle(pipe)


def process_id(value):
    """Accept only a local Windows process identifier, never an endpoint or path."""
    try:
        pid = int(value)
    except ValueError:
        raise argparse.ArgumentTypeError("PID must be a positive Windows process ID") from None
    if not 0 < pid <= 0xFFFFFFFF:
        raise argparse.ArgumentTypeError("PID must be a positive Windows process ID")
    return pid


def main():
    """Write exactly one JSONL result; failed bridge requests exit with status 1."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pid", required=True, type=process_id, help="local DK2 process ID")
    commands = parser.add_subparsers(dest="command", required=True)
    snapshot = commands.add_parser("snapshot", help="read a named game-thread snapshot")
    snapshot.add_argument("name", choices=("session", "player", "hand", "possession"))
    trace = commands.add_parser("trace", help="toggle or drain the bounded action trace")
    trace.add_argument("mode", choices=("on", "off", "drain"))
    logging = commands.add_parser("logging", help="temporarily toggle debug logging")
    logging.add_argument("category", choices=("debug",))
    logging.add_argument("mode", choices=("on", "off"))
    args = parser.parse_args()
    if args.command == "snapshot":
        command = f"snapshot {args.name}"
    elif args.command == "trace":
        command = f"trace {args.mode}"
    else:
        command = f"logging debug {args.mode}"
    try:
        result = request(args.pid, command)
    except BridgeError as error:
        result = {"ok": False, "error": str(error)}
    print(json.dumps(result, separators=(",", ":")), flush=True)
    return 0 if result["ok"] else 1


if __name__ == "__main__":
    sys.exit(main())
