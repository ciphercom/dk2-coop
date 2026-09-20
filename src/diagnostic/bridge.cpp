#include "bridge.h"

#include <cstdlib>

namespace patch::diagnostic {
namespace {
enum class Operation { snapshot, traceOn, traceOff, drain, loggingOn, loggingOff };
struct Command { std::string_view wire; Operation operation; Snapshot snapshot = Snapshot::session; };
// Every permitted operation is registered here; input never selects an address, function or config path.
constexpr Command commands[] = {
    {"snapshot session", Operation::snapshot, Snapshot::session},
    {"snapshot player", Operation::snapshot, Snapshot::player},
    {"snapshot hand", Operation::snapshot, Snapshot::hand},
    {"snapshot possession", Operation::snapshot, Snapshot::possession},
    {"trace on", Operation::traceOn}, {"trace off", Operation::traceOff}, {"trace drain", Operation::drain},
    {"logging debug on", Operation::loggingOn}, {"logging debug off", Operation::loggingOff},
};
}

std::string error(std::string_view code) {
    return "{\"ok\":false,\"error\":\"" + std::string(code) + "\"}\n";
}

/** Serialize only a present cached target; absent target storage in DK2 may be uninitialized. */
std::string handEntryJson(uint16_t tag, int dropped, const std::optional<HandDropTarget> &target) {
    std::string result = "{\"tag\":" + std::to_string(tag) + ",\"has_under_hand\":" + (target ? "true" : "false") +
        ",\"dropped\":" + std::to_string(dropped) + ",\"drop_target\":";
    if (!target) return result + "null}";
    return result + "{\"x_if12\":" + std::to_string(target->x) + ",\"y_if12\":" + std::to_string(target->y) +
        ",\"type\":" + std::to_string(target->type) + ",\"tag\":" + std::to_string(target->tag) + "}}";
}

/** Retain recent observations and account for every overwritten event. */
void Core::record(const Action &action) {
    if (!tracing_) return;
    if (count_ == traceCapacity) {
        first_ = (first_ + 1) % traceCapacity;
        --count_;
        ++dropped_;
    }
    events_[(first_ + count_++) % traceCapacity] = {++sequence_, action};
}

/** Exact matching makes unsupported operations fail before any provider is called. */
std::string Core::execute(std::string_view command, const SnapshotProvider &snapshot, const LoggingToggle &logging) {
    const Command *entry = nullptr;
    for (const auto &candidate : commands) if (candidate.wire == command) entry = &candidate;
    if (!entry) return error("unknown_command");
    switch (entry->operation) {
    case Operation::snapshot:
        return "{\"ok\":true,\"snapshot\":\"" + std::string(command.substr(9)) +
            "\",\"data\":" + snapshot(entry->snapshot) + "}\n";
    case Operation::traceOn:
    case Operation::traceOff:
        tracing_ = entry->operation == Operation::traceOn;
        return std::string("{\"ok\":true,\"trace\":") + (tracing_ ? "true}\n" : "false}\n");
    case Operation::loggingOn:
    case Operation::loggingOff: {
        const bool enabled = entry->operation == Operation::loggingOn;
        logging(enabled);
        return std::string("{\"ok\":true,\"logging\":\"debug\",\"enabled\":") + (enabled ? "true}\n" : "false}\n");
    }
    case Operation::drain: {
        std::string result = "{\"ok\":true,\"events\":[";
        for (size_t i = 0; i < count_; ++i) {
            const auto &[sequence, action] = events_[(first_ + i) % traceCapacity];
            if (i) result += ',';
            result += "{\"sequence\":" + std::to_string(sequence) + ",\"boundary\":\"";
            result += action.boundary == Boundary::local_queue_before_handle ? "local_queue_before_handle" : "world_tick_return";
            result += "\",\"tick\":" + std::to_string(action.tick) + ",\"action_type\":" + std::to_string(action.actionType) +
                ",\"player_id\":" + std::to_string(action.playerId) + ",\"data1\":" + std::to_string(action.data1) +
                ",\"data2\":" + std::to_string(action.data2) + ",\"data3\":" + std::to_string(action.data3) + "}";
        }
        result += "],\"dropped\":" + std::to_string(dropped_) + "}\n";
        count_ = first_ = 0;
        dropped_ = 0;
        if (result.size() > responseLimit) std::abort(); // A fixed trace must always fit the promised budget.
        return result;
    }
    }
    std::abort(); // An enum without a handler is a programmer error.
}
}
