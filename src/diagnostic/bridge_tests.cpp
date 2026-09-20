#include "bridge.h"
#include "graphics_lifecycle.h"

#include <cstdio>
#include <cstdlib>
#include <string>

namespace {
/** Keep checks active in production-config test builds too. */
void require(bool condition, const char *message) {
    if (!condition) {
        std::fprintf(stderr, "%s\n", message);
        std::exit(EXIT_FAILURE);
    }
}
}

/** Exercise the public diagnostic contract without depending on DK2 memory layouts. */
int main() {
    using namespace patch::diagnostic;
    require(handEntryJson(9, 0, std::nullopt) ==
            "{\"tag\":9,\"has_under_hand\":false,\"dropped\":0,\"drop_target\":null}",
            "Hand entries without a cached target must not expose uninitialized target fields");
    require(handEntryJson(9, 1, HandDropTarget{-4096, 8192, 2, 17}) ==
            "{\"tag\":9,\"has_under_hand\":true,\"dropped\":1,\"drop_target\":{\"x_if12\":-4096,\"y_if12\":8192,\"type\":2,\"tag\":17}}",
            "pending Hand state must preserve target coordinates, type, tag and dropped flag");
    Core core;
    unsigned snapshotCalls = 0;
    bool debugLogging = false;
    auto snapshot = [&](Snapshot name) {
        ++snapshotCalls;
        require(name == Snapshot::session, "registered snapshot must reach its typed provider");
        return std::string("{\"world_active\":false}");
    };
    auto logging = [&](bool enabled) { debugLogging = enabled; };
    require(core.execute("snapshot session", snapshot, logging) ==
            "{\"ok\":true,\"snapshot\":\"session\",\"data\":{\"world_active\":false}}\n",
            "named snapshots must preserve provider data in the JSON envelope");
    require(core.execute("snapshot memory 0x1234", snapshot, logging) ==
            "{\"ok\":false,\"error\":\"unknown_command\"}\n", "raw access must be rejected");
    require(core.execute("trace on extra", snapshot, logging).find("unknown_command") != std::string::npos,
            "the command registry must reject trailing arguments");
    require(snapshotCalls == 1, "invalid requests must not read game state");
    core.record({Boundary::local_queue_before_handle, 12, 4, 2, 11, 22, 33});
    require(core.execute("trace drain", snapshot, logging) ==
            "{\"ok\":true,\"events\":[],\"dropped\":0}\n", "trace must default off");
    require(core.execute("trace on", snapshot, logging) == "{\"ok\":true,\"trace\":true}\n",
            "trace on must acknowledge the enabled state");
    for (unsigned i = 0; i < 130; ++i)
        core.record({Boundary::world_tick_return, 12, 4, 2, i, 22, 33});
    auto drained = core.execute("trace drain", snapshot, logging);
    require(drained.find("\"dropped\":2") != std::string::npos, "overflow must be counted");
    require(drained.find("\"sequence\":3,") != std::string::npos,
            "overflow must retain the most recent 128 events in sequence order");
    require(drained.find("\"sequence\":130,") != std::string::npos, "the newest event must survive overflow");
    require(drained.find("\"boundary\":\"world_tick_return\",\"tick\":12,\"action_type\":4,\"player_id\":2,\"data1\":129,\"data2\":22,\"data3\":33") != std::string::npos,
            "traces must retain action identity, arguments and truthful observation boundary");
    require(drained.size() <= responseLimit, "the maximum trace must fit the response budget");
    require(core.execute("trace drain", snapshot, logging) ==
            "{\"ok\":true,\"events\":[],\"dropped\":0}\n", "drain must consume events and overflow count");
    core.execute("trace off", snapshot, logging);
    core.record({Boundary::local_queue_before_handle, 13, 4, 2, 0, 0, 0});
    require(core.execute("trace drain", snapshot, logging).find("\"events\":[]") != std::string::npos,
            "trace off must stop recording");
    core.execute("logging debug on", snapshot, logging);
    require(debugLogging, "the registered logging toggle must reach its typed provider");
    core.execute("logging debug off", snapshot, logging);
    require(!debugLogging, "logging must be disableable without restarting");

    // Graphics lifecycle churn cannot consume the one reserved missing-scratch observation.
    GraphicsLogBudget graphics;
    for (unsigned i = 0; i < 140; ++i) {
        require(!graphics.take(false, false), "disabled graphics lifecycle logging emitted output");
        require(!graphics.take(false, true), "disabled fault logging consumed its reserved line");
    }
    for (unsigned i = 0; i < 127; ++i)
        require(graphics.take(true, false), "graphics lifecycle budget exhausted early");
    require(!graphics.take(true, false), "normal graphics logging consumed the reserved fault line");
    require(graphics.take(true, true), "first missing-scratch fault was lost after lifecycle churn");
    require(!graphics.take(true, true) && !graphics.take(true, false), "graphics logging exceeded 128 lines");
    GraphicsLogBudget earlyFault;
    require(earlyFault.take(true, true), "first graphics fault was omitted");
    for (unsigned i = 0; i < 127; ++i)
        require(earlyFault.take(true, i % 2 == 0), "fault-first trace exhausted its shared budget early");
    require(!earlyFault.take(true, true) && !earlyFault.take(true, false),
        "repeated faults escaped the process output bound");
    return 0;
}
