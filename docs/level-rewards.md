# v4 level rewards

Account XP comes from fish settlement, with zero XP for fish and decor purchases. Account XP caps at level 40. Lifetime XP can keep increasing.

Each crossed level pays the exact coins and pearls in the workbook's `XP & Unlocks` sheet. Level 2 gives 500 coins and one pearl. The full table is generated as `level_rewards` in `assets/content.json`.

The level checkpoint, ledger entries and wallet changes commit with the fish settlement. Retrying the settlement or reopening its saved result cannot pay those levels again. The old level-up dialog has been removed. Wallet balances and the XP bar show the credited result; a replacement level-up dialog is not implemented.

Tests in `tests/level_reward_tests.cpp` cover all 39 grants, multi-level jumps, ordered notifications, cap behavior, reload replay and atomic rollback when a balance would overflow.
