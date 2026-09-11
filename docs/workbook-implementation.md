# Workbook implementation

The launch game now uses the retained workbook for its roster and mapped progression values. This pass excludes the tutorial, the Anniversary event date, and reward rules the workbook does not specify, as requested by the user. The Start guide entry is removed from Settings Help; unused tutorial code and saved state remain for compatibility.

## Import and independent verification

The source is `design/aquarium_game_master_model.xlsx`. `tools/import_content.py` imports the catalog, XP curve, starters, tank schedules and dated events into `assets/content.json`. It calls the shared supplement importer to add quest rules, exact XP columns, mastery targets and missing-specification notes before writing the complete file atomically. `tools/import_supplement.py` remains a compatible standalone command.

Each imported field records its source cell or derivation. `tools/check_catalog.py` independently reopens the workbook instead of relying on importer output as proof. The recorded run passed **1,672 comparisons**: 1,439 explicit fields and 233 derived fields, with no errors. See `evidence/workbook-catalog-check.log` for the result and unresolved source details.

Reproduce the import and check from the repository root:

```sh
python3 tools/import_content.py design/aquarium_game_master_model.xlsx
python3 tools/import_supplement.py
python3 tools/check_catalog.py --content assets/content.json --workbook design/aquarium_game_master_model.xlsx
```

The launch runtime supports levels 1 through 40. The workbook's levels 41 through 70 are retained as future content.

## Fish and Shop

All **46 fish** use their workbook identities: 26 coin fish, 10 premium fish and 10 limited fish. Imported values include names, model IDs, unlock levels, price currency, purchase XP, growth and care intervals, stage sale rewards, and event rules. Where the workbook lacks explicit Limited Edition intermediate sale values, the importer records the calculation and its source inputs separately.

A normal new game starts with hungry Baby Neon Tetra, Guppy, Platy and Molly, 250 coins, no Pearls and a 10-fish tank. The Level 2 Pearl grant comes from the workbook. Existing saves retain their fish and wallet. Screenshot fixtures remain separate review states and are not the normal starting roster.

The Shop lists the catalog in group and unlock order, with Coin, Premium and Limited filters. Cards show unlock level, rarity, stage and feeding intervals, price, and seasonal dates where configured. Button labels explain level, event, capacity, currency and claim blockers. The domain checks those same restrictions again before a purchase changes state.

## Quests and mastery

| Objective | Target | Unlock | Status |
|---|---|---:|---|
| Feed Caretaker | Feed 8 times while no living, unstashed fish is sick | Level 2 | Exact XP claim enabled; sickness resets incomplete progress; completed goals stay ready |
| Junior Seller | Sell 2 eligible fish | Level 2 | Exact XP claim enabled |
| Adult Harvest | Sell 1 Adult fish | Level 3 | Exact XP claim enabled |
| Tank Stylist | Place 1 decor item | Level 3 | Exact XP claim enabled |
| Collection Chapter | Complete the source's five-fish collection objective | Level 8 | Deferred pending themes, membership and rewards |
| Neighbor Helper | Send 5 gifts | Level 5 | Deferred pending gift rules and rewards |

Quest XP comes from the named columns in `Quest Scaling` and refreshes when the player levels up. The estimated daily coin pool is stored separately; it is not a per-quest payout. Daily and weekly claim resets use the calendar rules in `assumptions.md`.

Collection tracks discovery across all 46 species. Mastery counts fish reaching Adult, once per fish, with badges at **5, 25 and 100 Adults per species**. Selling a fish does not add Adult mastery. The new `adultRaised` save field and target-specific claim keys preserve progress. Old sales history is retained without converting it into unproven Adult growth.

Mastery XP, statue rewards and collection chapter rewards remain deferred. The badges do not grant invented currency.

## Tanks

The workbook's five tank unlock levels are 1, 7, 16, 25 and 34. Each tank progresses through 10, 20, 30 and 40 slots. On the solo path, each step requires **coins plus Gift Tokens** from the corresponding source entries. The friend-assist alternative is unavailable; there is no Pearl conversion.

The source names Medium and Large unlock beats but does not map every capacity step to an unlock level. Those entries are retained as unconfirmed metadata. The runtime enforces the clear tank unlock, ownership, capacity and cost rules without inventing missing expansion gates.

The existing local decor catalog remains available, with its unsourced numeric defaults listed in `assumptions.md`.

## Events and deferred grants

Nine limited fish have explicit source date windows. Endpoints are inclusive, including Frost Angelfish's window across New Year. Anniversary Rainbowfish has no specified calendar dates, so it stays unavailable until configured.

The old invented daily NPC token grant and egg exchange have been removed. Gift Tokens already present in a save remain valid. New gifts, Daily Egg Basket rewards, weekly collection rewards, Neighbor Helper rewards and other unspecified payouts remain unavailable. The user deferred these rules instead of requesting invented replacements.

## Species art

The canonical location is `assets/species/<species-id>.png`. The roster uses six suitable existing species sprites and 40 new species-specific generated sprites. Final asset and visual checks are tracked separately from the domain results. Prompts, source paths and visual acceptance notes are stored under `assets/species/provenance`.

The runtime normalizes sprites to face left, crops to their visible alpha bounds, caps the long edge at 512 pixels, and creates matching mask and desaturated dead-fish textures. These are runtime derivatives; the original PNGs remain unchanged. The tank, Shop, inventory and collection use the same canonical species art, avoiding unrelated fish substitutions.

## Recorded verification

- The independent catalog audit passed 1,672 workbook comparisons.
- The domain and storage suite passed 29 of 29 cases.
- The importer suite passed all 3 tests, including atomic failure and deterministic re-import.
- Asset validation passed all 46 canonical PNGs. Native renderer tests checked all 138 normal, mask and dead variants.
- Native touch tests passed at 804x415, 852x393 and 667x375. They cover every Shop and Collection entry, purchase gates, mastery, the modal backdrop, tank expansion with both currencies, placement, food, move, stash, restore and sale.
- The iPhone 17 Pro simulator build was installed and reviewed in landscape. The Shop and Collection rendered the real catalog, backdrop taps did not activate Food, and a Neon Tetra purchase charged 7 coins and granted 2 XP on placement.

Logs and screenshots are in `evidence/workbook-*`. Compact captures of all eight Shop pages and all Collection pages are in `evidence/workbook-catalog/667x375`. Visual review found and corrected whole-day rounding in Shop schedule labels.

This records simulator and desktop-native checks. Physical iPhone and Android testing and sustained mobile performance measurements remain separate work.
