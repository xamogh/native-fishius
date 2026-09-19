# Save format 4

New development games use `save-v4.json`. Both schema and content versions are 4. Earlier schema versions are not supported. The decoder rejects other versions; recovery preserves a corrupt or unsupported file and can load a separate valid backup.

All fish save an immutable `purchase` snapshot, identity, stage, growth milliseconds, hatch deadline, last-fed time, position, motion, favorite, scripted and storage fields. Six seconds of automatic hatching count toward total growth. Hunger pauses elapsed growth. Pearl purchases have zero refundable coin principal.

The save also contains the wallet, capped account XP, lifetime XP, level reward checkpoint, owned tanks, one shared fish capacity per tank, claims and counters, decor ownership and copies, settings, monotonic identity sequences, simulation and wall-clock anchors, transaction receipts, terminal settlements and ledger entries. Decorative copies save position, size, flip and stored state. A queued paid decor copy reserves its identity and inventory space.

New saves mark both `fishCapacity` and `fishLifecycle` as `shared` and contain one `fish` array. Older v4 `companions` records are read only for conversion. These become ordinary adults, preserving identity, favorites, position, movement and storage, with a sale quote frozen at the saved account level. Adults whose Keep reward was already paid receive new identities and zero-value quotes; their old settlements and receipts remain intact. Conversion changes no balances or XP. If an older save's separate capacities exceed a tank's shared limit, excess adult records move to storage. Current saves reject capacity overflow and do not write a `companions` array.

## Transactions

A request receipt stores the command fingerprint, result and revision. An identical request ID replays the stored result. Different data with the same ID is rejected. A settlement stores the fish snapshot, stage, disposition, principal refund, profit, actual account XP, request ID and timestamp. Sales use the unique fish key, so retries cannot pay twice. The old Keep command value 33 is retired, and later command values remain unchanged for existing receipts.

Eligible adult coin-fish sales also increment `totalEvents["adult-coin-sale"]`. Each such settlement stores a `pearl_reward` snapshot with its sale number, target and milestone amount. The twentieth sale grants one pearl in the same transaction as its coin and XP reward. The decoder checks eligibility, unique sale numbers and the saved milestone calculation. Historical settlements without this field keep zero pearl rewards. New tuning does not reprice their results, and old saves start with zero progress toward repeatable pearls.

Ledger entries contain currency, signed amount, balance after the change, reason, source ID, request ID, timestamp and revision. Each crossed level has its own reward source. The decoder checks unique sources, receipts, IDs, bounded values, stage consistency, capacity, historical adult origins and exact wallet reconciliation.

`Domain::execute(command, commit)` calculates a candidate and calls the supplied commit function. `Session::command` supplies local storage. If saving fails, domain state, receipts, loose food and pending presentation events revert to their previous values. Successful retries after a failed save remain possible. Move previews and loose food stay local; regular checkpoints save movement and free feeding progress.

Writes use a temporary file, flush, file synchronization on POSIX, backup and rename. The Windows branch still requires platform verification because removing the destination before rename does not give the POSIX replacement guarantee. A storage error remains visible until a successful checkpoint. Recovery uses a separate path and does not overwrite the corrupt primary.

Offline time is applied once from the saved wall-time anchor. A backward clock does not reverse growth. A fish can use only the fed portion of the absence. The current local save is not an anti-cheat mechanism. A future backend must own the clock, request identity and atomic commit.

All fish in tanks share a maximum of 200 spaces across five fully expanded tanks. Each tank's saved capacity must match a catalog step: 10, 15, 20, 25, 30, 35 or 40. Existing saves retain their purchased capacity and can buy the remaining steps without further level requirements. The v4 record limits are 10,000 fish including stored adults, 500 decor copies, 50,000 request receipts and 200,000 ledger entries. Receipt retention and compaction need a defined policy before a public release. Gestures, textures, active tools, loose pellets and on-screen reward animations are transient.
