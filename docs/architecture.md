# Architecture

`Content` holds schema 4 configuration from the v4 workbook. `Domain` owns the game state, care clocks, movement, free food and accepted commands. `src/economy.cpp` owns integer quote calculations, request receipts, the ledger and terminal Keep/Rehome settlements. `src/decor.cpp` owns decorative placement and ownership. These modules have no SDL or filesystem dependency.

A productive fish carries a purchase snapshot. Its future reward does not depend on the current catalog or account level. A companion has a separate ownership record without economic production fields. Both use the same visual movement through a temporary render representation. Care advances across owned tanks; only the active tank moves visually. The movement step is 20 milliseconds, while growth uses exact millisecond intervals.

`Session` supplies elapsed time and local persistence. An economic command succeeds only after its complete candidate save is committed. Failed commits roll back state and emitted events. `Domain::execute` accepts a commit callback, which provides a clear boundary for a future server. Execution is currently serial on the game thread.

`Storage` serializes explicit version 4 fields, validates decoded candidates, reconciles balances and uses temporary-file replacement with backups. There is no legacy migration. Offline growth consumes only the remaining meal duration and is applied once per saved wall-time interval.

`View` owns menus, tools, gesture ownership, temporary purchase offers, popover selection and confirmations. It retains IDs across commands, then looks them up again. `Canvas` owns SDL resources, fonts, texture caches and rendering. Rehome previews use the domain reward function and require confirmation of the displayed amount. Opening or drawing a popover cannot settle a fish.

Fish details use an anchored compact popover with no dark backdrop. The aquarium Food tool remains available. Full menus and dialogs retain their existing input ownership, spring motion and Reduced Motion behavior. Growing and display counts are shown separately. Missing artwork and deferred catalog releases are excluded from preloading and live offers.

Daily quest rewards, mastery rewards, projects, event catch-up, new onboarding, store checkout and online services remain separate later steps. Retained data or disabled prototype types do not mean those features are implemented.
