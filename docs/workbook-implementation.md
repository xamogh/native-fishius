# v4 implementation, steps 1 to 3

The active source is `design/aquarium_game_design_v4.xlsx`, copied from the supplied v4 workbook. The earlier `aquarium_game_master_model.xlsx` remains a historical reference. This record supersedes earlier implementation notes for these three steps.

## 1. Content foundation

`tools/import_v4.py` generates the complete schema 4 `assets/content.json`. Each species and decor item retains its source sheet and row. The workbook hash, configuration identity, source sheets, schedules, role factors, account levels, exact level grants and 35 tank entitlements are included. Regeneration is deterministic and replaces the output only after validation succeeds.

The catalog contains 72 coin fish, including 40 launch, 7 reserve and 25 future entries, plus 27 pearl fish and 120 decor items. Shop and Collection show launch entries with available artwork through level 40. Missing artwork, reserve entries and unscheduled seasonal fish remain closed. Asset validation checks the ready images separately. New artwork for the promoted roadmap species remains needed before those offers can open.

`tools/check_catalog.py` checks workbook rows independently, using rational arithmetic for rewards. It reports cached rounding discrepancies in `evidence/v4-implementation/catalog-audit.json`. Rule R25 specifies half-up ROUND and FLOOR partial claims. The imported runtime follows that rule, while `cached_*` fields preserve original workbook values. Three base profit values and some derived partial payouts differ by one from the workbook cache. The 17 September 2026 sale-rule update changed nine rule-text cells without changing numeric values, formulas or formatting.

The 18 September update removes Keep and separate fish capacities from 23 workbook sheets. Favorites block sales; fish stay until sold. The retired `showcase_slots` input is zero. Prices, sale rewards, XP, tank costs and workbook formatting are preserved. The original simulation snapshots and baseline checksum remain intact and are marked stale, because a rerun must account for favorite and premium fish using normal tank space. The generated content includes these revised rules. Workbook checks and phone captures are recorded in `evidence/fish-favorites-2026-09-18`.

User decisions override the source where recorded in `overrides`: free feeding pauses growth when hungry, meals last half the total schedule capped at 12 hours, and eggs hatch after six seconds. Save schemas before v4 remain unsupported, and there is no real backend.

The later 18 September tank decision overrides the workbook's capacity gates. Only new tanks require levels. Owned tanks can expand in steps of five to 40 fish. The importer preserves the original purchase prices through 20 fish and adds four priced steps. The workbook's original tank rows remain in `raw_sheets`; their level schedule still supplies Treasure's reference income, so pack amounts do not change. See [tank upgrades](tank-upgrades.md) for the prices and override rules.

The 18 September decor XP decision overrides the workbook's zero purchase XP for plants and decorations. The first copy of each item type grants one XP per 10 coins, rounded half up with a minimum of one, or 10 XP per pearl. The importer records this rule in `overrides.decor_purchase_xp` and derives each item's `buy_xp` from its price. Ownership prevents repeat rewards across tanks, storage and reloads. Existing saves receive no retroactive XP. See [plants and decorations](plants-and-decor.md#purchase-xp) for the complete rule.

The later active-play balance pass records new fish XP rates, lower pearl egg prices, repeatable pearl earnings and smoother tank prices in `overrides.active_play`. These are intentional differences from the preserved workbook. Level thresholds, level grants and existing fish snapshots remain unchanged. The catalog audit distinguishes XP tuning changes from rounding corrections. See [Active-play economy](active-economy.md).

## 2. Local transaction foundation

Each fish records its purchase level, configuration identity, schedule, actual coin principal, profit, XP, stage thresholds and feeding interval. Coin prices and rewards use the level at purchase. Pearl prices come from the catalog, and pearl purchases have zero refundable coin principal. Later level or configuration changes do not reprice an owned fish.

Commands have request IDs and saved result receipts. Repeating a request returns its first result. Reusing an ID with different command data fails. A sale has one terminal settlement per fish, so different requests cannot collect its reward twice. The former Keep action is retired. The ledger records each wallet and account XP change, its reason, source identity and resulting balance.

`Session::command` commits the entire candidate save before returning success. A failed write rolls back the fish, wallet, receipt, progress and emitted events. The callback on `Domain::execute` is the boundary for a future authoritative service. The current game runs commands on one local game thread; it does not claim server security or cross-device concurrency support.

Schema 4 saves use `save-v4.json`. They require explicit current fields and reconcile the wallet against the ledger. Earlier v4 adult-only records convert into ordinary fish without granting rewards on load. Save schemas before v4, token conversion and legacy egg recovery remain unsupported.

## 3. Fish lifecycle and compact UI

- A normal new game has two Neon Tetras, one Guppy and one Platy, 250 coins, zero XP and a 10-slot tank. Gifted starters have zero refundable principal.
- Eggs hatch after six seconds and start hungry. Those six seconds count toward the total growth schedule. Growth after hatching advances only while fed. There is no sickness, death, upkeep or revival charge.
- Schedules total 20 minutes, 2 hours, 8 hours, 20 hours or 44 hours. After hatching, Baby, Junior, Young, Mature and Adult use 0%, 25%, 55%, 80% and 100% of the total duration. A meal covers half the schedule, capped at 12 hours.
- Food remains in the aquarium toolbar and uses the existing pellet interaction. It costs nothing and grants no coins or XP.
- Sell shows the actual current-stage coin and XP payout. Eggs and Babies cannot be sold. Selling unlocks at Junior (age 1 / 25% growth). Junior, Young and Mature use the saved principal refund and partial profit rules; Adult pays the maximum. Historical baby and egg refunds remain valid on save load and replay, but no new ones are allowed.
- Fish stay in the tank at adulthood. Favoriting protects them from selling without granting coins or XP or freeing space. Sell grants the saved current-stage reward and removes the fish. There is no Keep action or separate display capacity. Eggs, adults and premium fish share the tank limit. Previously rewarded adults remain owned without receiving another reward.
- All fish use the same lifecycle. Bubble Eye Goldfish costs one pearl at level 2 and permits repeat purchases. The 27 former companion definitions use the balanced two-hour schedule and normal coin/XP sales as a recorded runtime override. Active-play tuning divides other pearl prices by six, rounding up. Release gates remain unchanged. Buying fish gives zero account XP. Plants and decorations grant their first-purchase bonus. Rarity does not multiply production.
- Tapping a fish opens a compact popover with growth, care, current coin and XP sale value, and the adult comparison. Its heart toggles favorite protection. A single Sell button sells eligible fish immediately at their current value. The sell net also sells eligible fish with one tap. Eggs, Babies and favorites cannot be sold.
- New tanks retain their workbook level gates and ownership order. Owned tanks expand through 10, 15, 20, 25, 30, 35 and 40 fish spaces at any level, with either the coin or pearl price. Five fully expanded tanks hold 200 fish. Account XP caps at level 40; lifetime XP can continue. Level grants are paid once at each crossed level.

Daily quest pools, mastery rewards, collection chapters, projects, event catch-up, the new scripted onboarding, ads, purchases through a store provider and server deployment remain later steps. Their workbook data is retained. Their previous economic grants are disabled.

## Verification

Run the CMake desktop test preset, `python3 tools/test_content_import.py`, `python3 tools/check_catalog.py` and `python3 tools/validate_assets.py`. The new lifecycle suite covers saved quotes, rounding boundaries, six-second hatch, hunger, offline time, sale retries, favorite retention, replay after reload, save failure, shared fish capacity, tank prices and account XP caps. Existing native UI suites cover mouse and touch input, menus, purchases, movement, decor and rendering under the current rules.

Phone and tablet popover captures are in `evidence/v4-implementation`. This implementation has desktop build and software-renderer checks. This turn does not verify physical iOS or Android devices or deploy a server.

Historical icon generation records remain in `assets/ui/provenance/keep.json` and `assets/ui/provenance/rehome.json`. The removed Keep action does not use its former icon.

Verified results are recorded in `evidence/v4-implementation/verification.json`: 19 desktop suites, five ASan/UBSan core suites, two importer regressions, 2,093 independent catalog checks and the decor audit. Follow-up suites cover the final save and layout adjustments. Leak detection is unavailable on this host and was disabled.
