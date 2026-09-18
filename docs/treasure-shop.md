# Treasure offers

The active v4 workbook defines five coin packs and five pearl packs. Both the Clay Treasure page and the earlier currency shop read the same generated catalog.

| Size | Reference coin-days | Coin price (USD) | Pearls | Pearl price (USD) |
| --- | ---: | ---: | ---: | ---: |
| Pocket | 0.25 | $0.99 | 20 | $1.99 |
| Pile | 0.85 | $2.99 | 55 | $4.99 |
| Bag | 1.5 | $4.99 | 120 | $9.99 |
| Box | 3.25 | $9.99 | 260 | $19.99 |
| Chest | 7 | $19.99 | 420 | $29.99 |

These are proposed USD prices. Payment processing is not connected. Opening an offer shows its exact contents and price and does not grant currency or charge the player.

## Coin amounts

Coin packs use the reference income for the current account level. Reference capacity is the sum of all growing slots eligible at that level, whether the player owns them or not. Buying capacity does not increase a pack's value at the same level.

Reference income is the base daily coin rate multiplied by the level factor, eligible slots and reference utilization. Multiply this income by the pack's coin-days, then round once to the nearest whole coin, with halves rounded up. The runtime uses integer fractions for this calculation. Cards display full amounts with thousands separators.

At level 10, the five packs contain 3,161, 10,748, 18,968, 41,096 and 88,515 coins. Pearl amounts stay fixed at every level. Every Treasure offer has unlock level 0, meaning it is available from the start without a level or tutorial requirement. The account still starts at level 1; this change affects shop eligibility only.

## Starter bundle

The earlier currency shop keeps its separate starter card. It uses v4's contents: 35 pearls, 0.25 reference coin-day and a permanent frame for $3.99. It is available from the start, with a lifetime limit of one purchase per account. This is also a preview until checkout and entitlement tracking are connected.

## Artwork

Each currency tier has its own transparent illustration in `assets/treasure/`: a handful, pile, bag, box and chest. Gold star coins and iridescent pearls match the existing currency icons. The starter bundle has a separate illustration with coins, pearls and a decorative frame. Both shop views use the catalog's asset paths. `assets/treasure/generation-v1.json` records the built-in image generator, prompts and original output files.

## Source and checks

`Shop!A5:L18` contains the offers and purchase rules. Existing offer IDs are retained; the new tiers use `COIN-XL`, `COIN-XXL`, `PEARL-XL` and `PEARL-XXL`. `Balance Tests!D29` checks increasing pearl value across all five tiers.

`tools/import_v4.py` generates `treasure` in `assets/content.json`. `tools/check_catalog.py` compares pack contents and prices with the workbook and checks that each larger pack gives more value per dollar. The C++ tests cover whole-coin rounding, level changes, both currencies, invalid content and browsing without changing saved state.
