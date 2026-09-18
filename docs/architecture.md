# Architecture

`Content` holds schema 4 configuration from the v4 workbook. `Domain` owns the game state, care clocks, movement, free food and accepted commands. `src/economy.cpp` owns integer quote calculations, request receipts, the ledger and terminal Keep/Rehome settlements. `src/decor.cpp` owns decorative placement and ownership. These modules have no SDL or filesystem dependency.

A productive fish carries a purchase snapshot. Its future reward does not depend on the current catalog or account level. A companion has a separate ownership record without economic production fields. Both use the same visual movement through a temporary render representation. Care advances across owned tanks; only the active tank moves visually. The movement step is 20 milliseconds, while growth uses exact millisecond intervals.

`Session` supplies elapsed time and local persistence. An economic command succeeds only after its complete candidate save is committed. Failed commits roll back state and emitted events. `Domain::execute` accepts a commit callback, which provides a clear boundary for a future server. Execution is currently serial on the game thread.

`Storage` serializes explicit version 4 fields, validates decoded candidates, reconciles balances and uses temporary-file replacement with backups. There is no legacy migration. Offline growth consumes only the remaining meal duration and is applied once per saved wall-time interval.

The game has one interface. `main.cpp` connects the current HUD, Shop, tank switcher, dialogs and egg placement. `HudCare` owns feeding, rehoming, fish selection and confirmation. `hud_placement.cpp` owns egg placement. `hud_tank_switcher.cpp` owns the porthole switcher and tank purchases in Shop. Each action uses `Session`, so the UI cannot bypass save or economy checks.

`Canvas`, declared in `canvas.hpp`, owns SDL resources, fonts, texture caches and drawing. Clay computes the current HUD and dialog bounds. Rendering and input use those same bounds. `fish_details.cpp` supplies the current care dialog's reward and growth text. The old `View`, panel skins, standalone tank dialog and Studio editor have been removed from source and the build.

`--hud-layout` is accepted for compatibility and does not select an alternate UI. `--scene-only` remains a rendering preview. Review fixtures use an ephemeral session. Normal desktop, iOS and Android launches enter the same current interface.

Daily quest rewards, mastery rewards, projects, event catch-up, new onboarding, store checkout and online services remain separate later steps. Retained data or disabled prototype types do not mean those features are implemented.
