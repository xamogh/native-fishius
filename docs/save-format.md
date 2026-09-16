# Save format 4

New development games use `save-v4.json`. Both schema and content versions are 4. No backward compatibility is needed or implemented. The decoder rejects other versions; recovery preserves a corrupt or unsupported file and can load a separate valid backup.

Productive fish save an immutable `purchase` snapshot, identity, stage, growth milliseconds, hatch deadline, last-fed time, position, motion, favorite and scripted flags. Six seconds of automatic hatching count toward total growth. Hunger pauses elapsed growth. Companion records are separate and contain ownership and visual state, with no coin or XP production fields. Kept adults link to their original terminal settlement.

The save also contains the wallet, capped account XP, lifetime XP, level reward checkpoint, owned tanks, eight-slot display occupancy per tank, claims and counters, decor ownership and copies, settings, monotonic identity sequences, simulation and wall-clock anchors, transaction receipts, terminal settlements and ledger entries. Decorative copies save position, size, flip and stored state. A queued paid decor copy reserves its identity and inventory space.

## Transactions

A request receipt stores the command fingerprint, result and revision. An identical request ID replays the stored result. Different data with the same ID is rejected. A settlement stores the fish snapshot, stage, disposition, principal refund, profit, actual account XP, request ID and timestamp. Keep and Rehome share its unique fish key.

Ledger entries contain currency, signed amount, balance after the change, reason, source ID, request ID, timestamp and revision. Each crossed level has its own reward source. The decoder checks unique sources, receipts, IDs, bounded values, stage consistency, capacity, companion origins and exact wallet reconciliation.

`Domain::execute(command, commit)` calculates a candidate and calls the supplied commit function. `Session::command` supplies local storage. If saving fails, domain state, receipts, loose food and pending presentation events revert to their previous values. Successful retries after a failed save remain possible. Move previews and loose food stay local; regular checkpoints save movement and free feeding progress.

Writes use a temporary file, flush, file synchronization on POSIX, backup and rename. The Windows branch still requires platform verification because removing the destination before rename does not give the POSIX replacement guarantee. A storage error remains visible until a successful checkpoint. Recovery uses a separate path and does not overwrite the corrupt primary.

Offline time is applied once from the saved wall-time anchor. A backward clock does not reverse growth. A fish can use only the fed portion of the absence. The current local save is not an anti-cheat mechanism. A future backend must own the clock, request identity and atomic commit.

Limits are 100 growing fish, 10,000 owned companions, 500 decor copies, 50,000 request receipts and 200,000 ledger entries. Receipt retention and compaction need a defined policy before a public release. Gestures, textures, active tools, loose pellets and on-screen reward animations are transient.
