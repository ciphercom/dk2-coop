# Internal co-op port scope

DK2 Co-op carries the playable campaign work from the earlier local development workspace into a standalone fork of Flame. The port keeps the established native menu route, mission tagging/loading, host progression, shared Keeper actions, independent hand contents and local possession presentation, scripted cameras, gem endings, and focused regression checks. Original campaign assets remain unchanged.

The build and package workflow belongs to this repository and stages its output locally. It does not depend on neighboring game installations, overwrite personal settings or require a development launch profile. Public project, issue and release links point to `ciphercom/dk2-coop`. Flame's authorship, history, loader layout and bundled editor attribution are retained.

The original co-op release port excluded the diagnostic bridge and development-only availability overrides. This dedicated diagnostics branch adds the reusable bridge, opt-in test tools and logging hooks described in the [Flame adoption notes](development/diagnostic-bridge-port.md). Investigation captures and source-workspace test profiles remain excluded; native protocol/OOS facilities remain unchanged. Existing upstream facilities are not a prerequisite for playing co-op. Regression tests that express gameplay or package contracts remain part of the normal build.

This scope preserves the proven runtime implementation rather than creating a new campaign browser, progression store or networking service. [Internal port validation](development/port-validation.md) records inherited evidence, pending acceptance and scope decisions; [build instructions](build.md) describe the standalone release process.
