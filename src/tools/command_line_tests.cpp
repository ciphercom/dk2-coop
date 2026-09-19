#include "command_line.h"
#include <cstdio>
#include <cstdlib>

namespace {
/** Configuration regressions must fail in both Debug and Release builds. */
void require(bool valid, const char *message) {
    if (!valid) { std::fprintf(stderr, "%s\n", message); std::exit(EXIT_FAILURE); }
}
template<size_t N> void parse(const char *(&args)[N]) {
    cmdl::dict.clear(); cmdl::flags.clear(); cmdl::values.clear();
    command_line_init(int(N), args);
}
}

/** Exercise the actual launch parser without starting DKII or touching personal files. */
int main() {
    const char *normal[] = {"DKII-DX.exe", "-skip-launcher"};
    parse(normal);
    require(cmdl::dict.empty(), "ordinary launches must not receive co-op overrides");
    const char *coop[] = {"DKII-DX.exe", "-skip-launcher", "-coop"};
    parse(coop);
    // An obsolete launcher must not silently reintroduce the retired settings preset.
    require(cmdl::dict.empty(), "legacy co-op flag must not inject configuration overrides");
    const char *custom[] = {"DKII-DX.exe", "-coop", "-skip-launcher", "-windowed=false", "-c=custom.toml"};
    parse(custom);
    require(cmdl::hasFlag("skip-launcher"), "explicit launch flags must survive");
    require(cmdl::dict.at("windowed") == "false" &&
        cmdl::dict.at("c") == "custom.toml", "explicit values and custom config selection must survive");
}
