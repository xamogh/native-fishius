# Tank upgrades

The 18 September 2026 user decision keeps level requirements only for buying new tanks. It follows the owned-tank progression described in the [25 March 2010 FishVille player guide](https://lynspirations.blogspot.com/2010/03/fishville-tank-upgrade.html).

Tank 1 starts free with 10 spaces. Tanks 2, 3, 4 and 5 still require levels 7, 16, 25 and 34, respectively, plus ownership of the preceding tank. The preceding tank does not need to be fully expanded. Tank 6 remains unavailable.

An owned tank can buy each step in order: 10, 15, 20, 25, 30, 35 and 40 fish. Each purchase charges either its coin price or its pearl price. Insufficient funds or a failed save leaves capacity and currency unchanged. All fish and eggs share these spaces, including favorites and premium fish.

Tap an available tank card to review its next capacity in a confirmation dialog. Choose Coins or Pearls there to complete the purchase. Opening or closing the dialog costs nothing. The current tank is marked with a gold Current badge.

## Prices

The later active-play balance pass replaces the earlier workbook prices and added-step multipliers. Each tank uses the six upgrade prices below, multiplied by 1, 1.5, 2, 2.5 or 3 for Tanks 1 to 5. Both currencies round halves up. This curve is an implementation choice for playtesting; it is not taken from FishVille.

For Tank 1:

| Capacity after purchase | Coins | Pearls |
| --- | ---: | ---: |
| 15 | 300 | 2 |
| 20 | 900 | 3 |
| 25 | 1,800 | 4 |
| 30 | 3,000 | 5 |
| 35 | 4,500 | 6 |
| 40 | 6,300 | 7 |

Tank 1 costs 16,800 coins or 27 pearls to expand fully. Each step can use a different currency.

| New tank | Level | Coins | Pearls |
| --- | ---: | ---: | ---: |
| Tank 2 | 7 | 1,500 | 5 |
| Tank 3 | 16 | 6,000 | 10 |
| Tank 4 | 25 | 15,000 | 15 |
| Tank 5 | 34 | 30,000 | 20 |

## Content and saves

The importer records these decisions in `overrides` and generates 35 tank entitlements. Upgrade entries have level 0, meaning no additional level requirement. The original workbook rows stay intact in `raw_sheets`. The original level schedule is exported separately for Treasure reference income, preserving coin pack amounts. See [Active-play economy](active-economy.md) for the related fish XP and pearl-price changes. Level thresholds and level grants stay unchanged.

Existing v4 saves retain their purchased 10, 15 or 20 spaces. A saved 20-space tank can now buy the next upgrade. No capacity or purchase is granted automatically. Saves accept every new capacity step and up to 200 timed-growth fish across the five tanks. Level-up dialogs show new tanks, and the Tank Shop shows upgrades for owned tanks at any level.

Earlier workbook simulations used a 20-fish cap and level gates for expansion. They do not validate progression speed under these rules. The active-play cash-flow model buys actual capacity, reserves egg money and keeps favorites. Its assumptions and results are recorded in the active economy guide; they still need playtesting.
