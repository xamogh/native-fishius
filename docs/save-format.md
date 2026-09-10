# Save format and recovery

The save is JSON with explicit schema and content versions. The first native schema is version 1. Unsupported future versions are rejected and preserved, not silently converted into a new game.

The record contains the wallet, cumulative XP, highest rewarded level, simulation and wall-time anchors, owned tanks, active tank, fish identities, positions, lifecycle and paused-storage timers, relevant movement and random state, claims, local tutorial and quest state, discoveries, mastery, decor, and settings.

Transient pointers, textures, fonts, active pointer gestures, loose food, tool animations, and receipts are not serialized.

Validation checks known species, unique IDs, tank references, legal capacity steps, numeric bounds, finite positions, stage consistency, and bounded collection sizes. Living fish may exceed nominal tank capacity after revival. This is intentionally valid saved state. Ordinary purchase and restoration gates still reject a full or overflowing tank.

Writes use a temporary file, flush, and replacement, retaining a last-good backup where possible. POSIX writes also request file synchronization. The Windows replacement branch requires additional platform review; deleting an existing destination before rename is not the same crash-atomic guarantee as POSIX replacement.

A corrupt or unsupported primary is not overwritten just because a backup loads. Recovery uses a separate recovered path. Storage failures should be shown to the player and retained in logs rather than silently switching to session-only behavior.

Offline time is applied once from the persisted wall-time anchor. Negative wall-clock changes do not reverse simulation. Calendar reset anchors advance monotonically, limiting duplicate claims after a clock rollback. The local game does not provide server-authoritative anti-cheat protection against arbitrary system-clock changes.

There is no asserted migration from the unavailable original application's save format. The native schema is a separate format.
