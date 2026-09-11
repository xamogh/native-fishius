# Save format and recovery

The save is JSON with explicit schema and content versions. Both remain version 1. Unsupported future versions are rejected and preserved rather than silently replaced with a new game.

## Saved state

The record contains the wallet, cumulative XP, highest rewarded level, simulation and wall-time anchors, owned tanks, active tank, fish identities, positions, lifecycle and storage timers, movement and random state, claims, tutorial and quest state, discoveries, decor, and settings.

Progress fields include:

- `mastery`: historical per-species sale counts, retained for compatibility.
- `adultRaised`: per-species counts of fish that reached Adult under the new rules.
- `claims`: once-only rewards and milestone claims. Adult mastery keys use `mastery:<species>:<target>`, where target is 5, 25 or 100.
- `quests`: objective count and claim flag, with daily and weekly period anchors.
- `giftTokens`, `giftDay` and `eggDay`: existing balances and history, even though new gift and egg grants are deferred.

Transient pointers, textures, fonts, active gestures, loose food, tool animations and receipts are not serialized. Artwork updates do not change saved species IDs.

## Loading older native saves

`adultRaised` is optional when loading a version 1 save. If absent, it starts empty. Old sale counts included Junior fish and cannot prove that a fish reached Adult, so they are not converted to Adult mastery. Existing claims, currency and fish are retained.

The previous Feed Caretaker counter included feeds that do not satisfy the current healthy-feeding objective. When an older save has no `adultRaised` field, only an unclaimed `daily-feed` counter is reset. Claimed rewards remain claimed.

Loading a save does not replace its fish with the new starter roster or reset its wallet. The workbook starters apply only to a new game. Existing tutorial state remains available because tutorial work is deferred.

## Validation and recovery

Validation checks known species, unique IDs, tank references, legal capacity steps, numeric bounds, finite positions, stage consistency, and bounded collections. Living fish may exceed nominal capacity after revival. This is valid saved state; ordinary purchases and restoration still reject a full or overflowing tank.

Writes use a temporary file, flush and replacement, retaining a last-good backup where possible. POSIX writes also request file synchronization. The Windows replacement branch still needs platform review because deleting a destination before rename does not provide the same crash-atomic guarantee.

A corrupt or unsupported primary is not overwritten just because a backup loads. Recovery uses a separate recovered path. Storage failures should reach the player and logs rather than silently switching to session-only behavior.

Offline time is applied once from the saved wall-time anchor. A clock rollback does not reverse simulation. Calendar anchors advance monotonically to limit duplicate claims. This local game does not prevent cheating through arbitrary system-clock changes.

There is no asserted migration from the unavailable original application's save format. This is the native application's format.
