#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

/** Fixed, read-only diagnostic vocabulary shared by DK2 and the synthetic transport host. */
namespace patch::diagnostic {
constexpr size_t requestLimit = 128;
constexpr size_t responseLimit = 65536;
constexpr size_t traceCapacity = 128;
enum class Snapshot { session, player, hand, possession };
enum class Boundary { local_queue_before_handle, world_tick_return };

/** These are observations at a dispatch boundary, not acknowledgements of action success. */
struct Action {
    Boundary boundary;
    int tick;
    int actionType;
    uint16_t playerId;
    uint32_t data1, data2, data3;
};

using SnapshotProvider = std::function<std::string(Snapshot)>;
using LoggingToggle = std::function<void(bool)>;
std::string error(std::string_view code);

/** A cached drop target exists only when DK2 marks the local Hand entry as having one. */
struct HandDropTarget { int x, y, type; uint16_t tag; };
std::string handEntryJson(uint16_t tag, int dropped, const std::optional<HandDropTarget> &target);

/** Owned and called only by the game thread; the transport exchanges copied strings. */
class Core {
public:
    std::string execute(std::string_view command, const SnapshotProvider &snapshot, const LoggingToggle &logging);
    void record(const Action &action);
    bool tracing() const { return tracing_; }
private:
    struct Event { uint64_t sequence; Action action; };
    std::array<Event, traceCapacity> events_{};
    size_t first_ = 0, count_ = 0;
    uint64_t sequence_ = 0, dropped_ = 0;
    bool tracing_ = false;
};
}
