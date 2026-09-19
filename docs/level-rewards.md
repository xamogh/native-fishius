# v4 level rewards

Account XP comes from fish settlement and the first purchase of each plant or decoration type. Coin decor gives one XP per 10 coins, rounded to the nearest whole XP with halves rounded up and a minimum of one. Pearl decor gives 10 XP per pearl. Repeated decor purchases and all fish purchases give zero XP. Account XP caps at level 40. Lifetime XP can keep increasing. See [purchase XP](plants-and-decor.md#purchase-xp) for ownership and save rules.

Each crossed level pays the exact coins and pearls in the workbook's `XP & Unlocks` sheet. Level 2 gives 500 coins and one pearl. The full table is generated as `level_rewards` in `assets/content.json`.

The active-play tuning raises XP for newly purchased fish and adds one pearl for every twenty eligible adult coin-fish sales. Rewards shows that repeatable progress. Existing fish keep their saved quotes, and old sales do not receive retroactive pearls. Level thresholds and level grants remain unchanged. See [Active-play economy](active-economy.md) for prices, examples and validation.

Only new tanks appear as tank level unlocks. Capacity upgrades on owned tanks are available at any level and do not appear in the unlock gallery.

The level checkpoint, ledger entries and wallet changes commit with the fish settlement or decor purchase. Retrying the transaction or reopening its saved result cannot pay those levels again.

After the XP animation reaches a new level, the level-up dialog shows that level's credited coins and pearls, plus newly unlocked shop fish, plants, decorations and tank offers. Gift fish use their separate claim rules and are not listed as shop purchases. Unreleased items and missing artwork are omitted. The mobile-first receipt keeps its level badge, coins, pearls and Continue button in a fixed left column. Each currency shows its icon and amount only, without a plus sign or currency name. A single row of square illustrated cards scrolls horizontally on the right. Two full cards and a partial next card make swiping clear. Names sit below the item artwork. Position marks follow scrolling. Swipe sideways, drag, use a mouse wheel or trackpad, or press arrow and Page Up/Down keys. Home and End reach the first and last cards. Portrait layouts put the receipt above the gallery and Continue below it. Continue, the close button, Escape or a backdrop tap dismiss the receipt without changing the save. When one settlement crosses several levels, their dialogs appear in order and each gallery starts at the left.

Coins and pearls start flying from their reward icons to the matching HUD counters 500 milliseconds after the dialog appears. The counters count up as icons arrive. The dialog leaves both HUD destinations visible, including on short screens. The animation runs once per receipt and does not restart when scrolling. Reduced motion shows the saved balance immediately and uses a brief highlight after the same pause. Closing early shows the saved totals immediately. These are presentation changes only; the original settlement already credited the wallet.

The dialog keeps the shared blue frame and uses the approved mobile-first badge, fixed receipt column and horizontal card gallery, without the extra balance or purchase helper text. Review with `--fixture level-up`, `--fixture level-up-10` or `--fixture level-up-40`; these fixtures never overwrite ordinary progress.

Tests in `tests/level_reward_tests.cpp` cover all 39 grants, multi-level jumps, ordered notifications, cap behavior, reload replay and atomic rollback when a balance would overflow.
