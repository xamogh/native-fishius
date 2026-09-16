# Workbook fixes, 12 September 2026

Egg placement in this historical report is superseded by `docs/egg-placement.md`. New purchases no longer use a paid queue.

This follow-up addresses confirmed defects and rules already defined in the workbook. The user chose **“Fix only already-defined rules.”** Onboarding remains excluded. Reward amounts, event dates, recovery policies and other missing economy rules have not been invented.

The source remains `design/aquarium_game_master_model.xlsx`, SHA-256 `1b6fe414def144f7792f998a779ac027ac5fe6af6edb8d17380d6933f3c4e77c`. The [detailed audit](workbook-audit-2026-09-12.md) records the earlier state and its evidence. Its findings remain a historical record; the table below records the corrections made in this follow-up.

## Implemented corrections

| Audit finding | Correction | Rule or purpose |
|---|---|---|
| 1: Paid egg blocks care and selling | Only the egg placement tool consumes the paid queue. Done or Escape keeps the eggs available while fish selection, Food and Sell work. The player can resume placement after making room. | A full tank no longer traps the player in a failed placement loop or loses the purchase. |
| 2: Cancel destroys paid decor | Cancelling a paid preview stores its copy in Bag. A pending purchase reserves capacity within the 500-copy safety bound and the next identity, and cannot be overwritten by another purchase. | `Decor Guide!C19`: each purchase creates one permanent cosmetic copy. First-ownership XP stays tied to the first acquisition. |
| 5: Save failures and recovery are hidden | Session exposes persistent save failures, a retry action and dismissible recovery information. Successful autosaves clear the failure but do not discard an unread recovery notice. A failed save on quit offers Keep open by default, or Quit without saving. | Gameplay success no longer hides unsaved progress. Recovery information remains visible until the player dismisses it. |
| 7: Active event items are absent from Shop | Limited fish appear during their defined calendar windows. Seasonal decor appears during its explicitly configured event. Level-locked cards remain visible; purchase checks still enforce eligibility. | `Quests & Live Ops!C13` and `Decor Guide!C35`: active event content has a purchase route. No missing date is filled in. |
| 8: Tank Stylist completes on purchase | Buying or cancelling a preview gives no objective progress. Placing or restoring a copy, moving it to a new position or tank, changing its size, or flipping it advances the objective. An unchanged move or size does not count. | `Quests & Live Ops!C7` and `Decor Guide!C28`: place or rearrange an owned item, with no required new spend. |
| 10: Decor can disappear outside the tank | Placement uses the scaled footprint to keep items reachable, with oversized art bottom-aligned. Purchases, previews, moves, resizing and restores share the bounds. Loading corrects old out-of-view positions, including legacy items. | `Decor Guide!C45`, `C49` and `C62`: dimensions and anchors must support usable placement. |
| 11: Decor ignores layers and edit mode | Decor renders in Background, Midground, Foreground order, then by bottom anchor within a layer. Hit-testing follows that draw order. Arrange tank enters decorate mode; normal Select targets fish. Fish retain selection priority and render above decor. | `Decor Guide!C50`: use each item's layer and restrict decor editing to decorate mode. |
| 12: Settings show independent defaults | Sound volume reads saved state directly. Reduced Motion is an independent saved switch. Inert music controls are replaced with an unavailable label. | `Decor Guide!C58`: reduced motion is directly user controlled. Displayed settings reflect their active values. |
| 13: Koi and Flashlight visuals are missing | Koi use stable pattern variations derived from each fish identity. Flashlight Fish have a timed cheek glow. Reduced Motion keeps the still artwork without the light pulse. | `Premium Fish!X3` and `X7`: individual Koi patterns and timed Flashlight glow. |
| 15: Shop tab filenames are inconsistent | Shop uses the named tab assets that exist in the asset bundle. | Shop preparation must complete without requesting missing numbered tab files. |

Paid items keep their ownership across restarts. Cancelling a decor preview does not refund the purchase or award XP again. A queued egg starts its hatch timer only when dropped. Decor score still counts distinct placed item IDs in each tank, with no score from stored or duplicate copies.

The normal HUD stays hidden while placing a paid or stored item. Save warnings remain visible so a placement gesture cannot hide a storage failure.

## Rules still missing from the source

These are still implementation gaps. They are unchanged because the user limited this pass to already-defined rules.

| Audit finding | Undefined rule | Current effect |
|---|---|---|
| 3 | Friend-assist implementation | Tank capacity now uses either Coins or Pearls, so it no longer depends on a separate social material. |
| 4 | Early bankruptcy recovery eligibility, amount and repeat-claim limit | The early no-living-fish and insufficient-currency state can still have no recovery route. |
| 6 | Offers and payment products | A later user request on 12 September supplied the level-up direction. Levels 2 to 40 now grant one pearl and coins; see `level-rewards.md`. Offers and payment products remain undefined. |
| 8 | Per-quest daily coin split; collection themes, membership and rewards; Neighbor Helper rewards; mastery XP/statues; Decorator thresholds and rewards | Daily XP and defined objectives work. The missing rewards and weekly systems remain incomplete. |
| 9 | Basket tables and probabilities, retired-fish rules, event eggs, gift chains and crafting recipes | The corresponding reward and social flows remain unavailable. |
| 14 | Daily bonuses, friend visits, recurring food or maintenance costs and other recurring source/sink amounts | The implemented transactions do not yet form the complete economy described by the workbook. Its forecast is not a verified playthrough. |

No Anniversary Rainbowfish dates or seasonal decor schedule have been added. Unconfigured events remain closed. The source conflicts on Bubble Eye purchase policy, Anniversary sale XP, capacity-step unlock mapping and Feed Caretaker wording remain documented in [Source decisions and remaining assumptions](assumptions.md) and the audit. The workbook's weak PASS checks and hardcoded dashboard cost also remain open source-quality issues.

Levels 41–70, Tanks 6–8, Roadmap fish, decor above Level 40 and conditional future set badges remain outside launch scope. Onboarding was neither completed nor rebalanced.

## Validation

The desktop build passed. All 14 regression suites passed after correcting the seasonal test setup. The initial full run passed 13 of 14 suites; the new seasonal test tried to move the calendar backward, which the domain correctly prevents. Its chronology was corrected, and its currency check now stores the previous paid decoration before attempting another purchase. The focused rerun passed.

Coverage includes paid-copy preservation, pending egg restart and recovery from a full tank, actual feeding and sale while an egg waits, free decor restore, first-ownership XP, Tank Stylist timing, scaled placement bounds, old-save migration, render layers, premium fish effects, persisted settings, event boundaries, and persistent save/recovery feedback. Purchase and save controls were exercised with mouse and touch at 667×375 and 1024×768. Existing suites also cover mobile layouts, startup preparation, Shop swipes, currency dialogs, fish details, selection and motion.

The phone captures for queued eggs, save failure during placement, save recovery and Settings were visually reviewed. `git diff --check` passed. The workbook hash is unchanged. No production save was used, and nothing was pushed.

Current evidence is in [the verification record](../evidence/workbook-fixes-2026-09-12/verification.json), [initial test log](../evidence/workbook-fixes-2026-09-12/ctest-initial.log), [focused rerun](../evidence/workbook-fixes-2026-09-12/ctest-retest.log), and the screenshots in that folder. Earlier audit probes intentionally reproduce the defects and remain in `evidence/workbook-audit-2026-09-12/`; they are not passing regression evidence for the corrected behavior.

Desktop regression checks do not establish physical-device performance, live payment or social integration, or a playable economy after the still-undefined sources and recovery rules. No production save should be used for test fixtures.
