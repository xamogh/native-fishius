# v4 implementation, steps 1 to 3

The active source is `design/aquarium_game_design_v4.xlsx`, copied from the supplied v4 workbook. The earlier `aquarium_game_master_model.xlsx` remains a historical reference. This record supersedes earlier implementation notes for these three steps.

## 1. Content foundation

`tools/import_v4.py` generates the complete schema 4 `assets/content.json`. Each species and decor item retains its source sheet and row. The workbook hash, configuration identity, source sheets, schedules, role factors, account levels, exact level grants and 15 tank entitlements are included. Regeneration is deterministic and replaces the output only after validation succeeds.

The catalog contains 72 coin fish, including 40 launch, 7 reserve and 25 future entries, plus 27 permanent companion definitions and 120 decor items. Shop and Collection show launch entries with available artwork through level 40. Missing artwork, reserve entries and unscheduled seasonal fish remain closed. Asset validation checks the ready images separately. New artwork for the promoted roadmap species remains needed before those offers can open.

`tools/check_catalog.py` checks workbook rows independently, using rational arithmetic for rewards. It reports cached rounding discrepancies in `evidence/v4-implementation/catalog-audit.json`. Rule R25 specifies half-up ROUND and FLOOR partial claims. The imported runtime follows that rule, while `cached_*` fields preserve original workbook values. Three base profit values and some derived partial payouts differ by one from the workbook cache. The 17 September 2026 sale-rule update changes nine rule-text cells in the active workbook. All numeric values, formulas and formatting remain unchanged.

User decisions override the source where recorded in `overrides`: free feeding pauses growth when hungry, meals last half the total schedule capped at 12 hours, eggs hatch after six seconds, and there is no save migration or real backend.

## 2. Local transaction foundation

Each productive fish records its purchase level, configuration identity, schedule, actual principal, profit, XP, stage thresholds and feeding interval. Prices and rewards use the level at purchase. Later level or configuration changes do not reprice an owned fish.

Commands have request IDs and saved result receipts. Repeating a request returns its first result. Reusing an ID with different command data fails. Keep and Rehome also share one terminal settlement per fish, so different requests cannot collect both rewards. The ledger records each wallet and account XP change, its reason, source identity and resulting balance.

`Session::command` commits the entire candidate save before returning success. A failed write rolls back the fish, wallet, receipt, progress and emitted events. The callback on `Domain::execute` is the boundary for a future authoritative service. The current game runs commands on one local game thread; it does not claim server security or cross-device concurrency support.

Schema 4 saves use `save-v4.json`. They require explicit current fields and reconcile the wallet against the ledger. No older-save migration, token conversion or legacy egg recovery is implemented.

## 3. Fish lifecycle and compact UI

- A normal new game has two Neon Tetras, one Guppy and one Platy, 250 coins, zero XP and a 10-slot growing tank. Gifted starters have zero refundable principal.
- Eggs hatch after six seconds and start hungry. Those six seconds count toward the total growth schedule. Growth after hatching advances only while fed. There is no sickness, death, upkeep or revival charge.
- Schedules total 20 minutes, 2 hours, 8 hours, 20 hours or 44 hours. After hatching, Baby, Junior, Young, Mature and Adult use 0%, 25%, 55%, 80% and 100% of the total duration. A meal covers half the schedule, capped at 12 hours.
- Food remains in the aquarium toolbar and uses the existing pellet interaction. It costs nothing and grants no coins or XP.
- Rehome shows the actual current-stage coin and XP payout. Eggs and Babies cannot be sold. Selling unlocks at Junior (age 1 / 25% growth). Junior, Young and Mature use the saved principal refund and partial profit rules; Adult pays the maximum. Historical baby and egg refunds remain valid on save load and replay, but no new ones are allowed.
- At adulthood, Keep and Rehome pay the same reward exactly once. Both free the growing slot. Keep creates a separate nonproducing display fish. Each tank has eight display slots; if those are full, Keep stores the fish in Bag. Display fish can be moved, fed, stored and restored without producing rewards.
- Premium fish are permanent companions. Bubble Eye Goldfish is a free, once-only level 2 companion. Buying fish or decor gives zero account XP. Rarity does not multiply production.
- Tapping a fish or using the net opens a compact popover. It shows stage, total progress, hunger or remaining fed growth, current coins and XP, and the adult comparison. Adult actions use Keep and Rehome icons. Rehome requires a second tap in the same popover. If its payout changes while open, the confirmation refreshes before allowing settlement. A favorite prevents rehome. No Feed icon or care penalty appears in the popover.
- Tank entitlements use the exact level gates and either the coin or pearl price, with 10, 15 and 20 growing slots per tank and 100 across five tanks. Account XP caps at level 40; lifetime XP can continue. Level grants are paid once at each crossed level.

Daily quest pools, mastery rewards, collection chapters, projects, event catch-up, the new scripted onboarding, ads, purchases through a store provider and server deployment remain later steps. Their workbook data is retained. Their previous economic grants are disabled.

## Verification

Run the CMake desktop test preset, `python3 tools/test_content_import.py`, `python3 tools/check_catalog.py` and `python3 tools/validate_assets.py`. The new lifecycle suite covers saved quotes, rounding boundaries, six-second hatch, hunger, offline time, Keep/Rehome races, replay after reload, save failure, display capacity, tank prices and account XP caps. Existing native UI suites cover mouse and touch input, menus, purchases, movement, decor and rendering under the current rules.

Phone and tablet popover captures are in `evidence/v4-implementation`. This implementation has desktop build and software-renderer checks. This turn does not verify physical iOS or Android devices or deploy a server.

The new icon assets have generation records in `assets/ui/provenance/keep.json` and `assets/ui/provenance/rehome.json`. Their source pixels are preserved.

Verified results are recorded in `evidence/v4-implementation/verification.json`: 19 desktop suites, five ASan/UBSan core suites, two importer regressions, 2,093 independent catalog checks and the decor audit. Follow-up suites cover the final save and layout adjustments. Leak detection is unavailable on this host and was disabled.
