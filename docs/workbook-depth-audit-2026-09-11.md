# Spreadsheet implementation audit

Reviewed on 11 September 2026 against the current working tree.

The fish catalog, growth and care rules, stage rewards, launch XP curve, daily quest XP, and mastery counters are implemented. The full progression system is incomplete. The largest gaps are usable tank expansion, renewable Pearl sources, recovery after losing every fish, onboarding, collection rewards, social features, and decor progression.

This audit covers all 17 worksheets and their 589 populated rows. It compares the workbook with imported content, runtime actions, screens, persistence, and tests. A row copied into `raw_sheets` is retained design data, not proof that a player can use the feature. No workbook cells or game source files were changed for this audit.

Source: [aquarium_game_master_model.xlsx](/Users/amoghrijal/amogh/aquarium-native/design/aquarium_game_master_model.xlsx). SHA-256: `427db6aaed85f924424c6897f9054d5c513d8e33d180bbe7e838ba71bdf87b25`.

## Most important findings

| Priority | Finding | Effect on play | Work still needed |
|---|---|---|---|
| First | Tank prices work, but Gift Tokens cannot be earned. | A fresh save cannot expand its initial 10 slots or buy Tank 2, even after earning enough coins and reaching the required level. | Define token grants and deliver a usable solo earning path. Add friend assists separately. |
| First | Only the Level 2 grant supplies Pearls in normal play. | The player can afford one Bubble Eye Goldfish. The other nine premium species, and all seven Pearl-priced limited species, remain unaffordable without a preloaded balance. Spending the Pearl on revival also competes with that first premium purchase. | Define and implement the intended ongoing Pearl sources. IAP is one listed source, but free grants and event or collection rewards also need rules. |
| First | No recovery path protects a player who loses all fish with too little money. | A valid early-game purchase sequence can leave 3 coins, zero Pearls, and no living fish after neglect. No fish can be bought or revived. | Implement a recovery policy that satisfies the workbook's early bankruptcy guardrail. The reward amount, conditions, and abuse limits need a decision. |
| Next | Several reward systems are only partly connected. | Daily quests pay XP only. Mastery awards badges only. Weekly quests cannot be claimed. | Complete the missing reward definitions, then connect grants, claims, screens, and save handling. |
| Next | Decor placement exists, but decor progression does not. | Purchases give no XP. Item score values are unused. There are no thresholds, titles, themes, or background rewards. | Add score aggregation and display, item XP, and the intended unlock and achievement content. |
| Next | The first-session tutorial is not available in normal play. | The player does not receive the spreadsheet's guided sequence, tutorial XP, free growth demonstration, social introduction, or return reward. | Build the 12-step flow and resolve its remaining reward details. Earlier project notes record this as deferred. |

These priorities are based on dependency and player impact. They are not estimates of development time.

## Worksheet-by-worksheet comparison

“Implemented” below applies to the named behavior or mapped values. “Partial” means some code or content exists but the complete player feature does not. “Needs specification” identifies an actual missing rule, rather than treating every missing feature as blocked on design.

| Worksheet | Implemented or retained | Remaining work and status |
|---|---|---|
| **Overview** | The target-rate catalog replaces the old purchase-price multipliers. Stage-specific XP and proportional sickness grace are active. | **Partial.** Mastery depth, social play, decor progression, recurring sinks, and weekly rewards described in B5:B16 are incomplete. Level 70 is deliberately future content. |
| **Inputs** | Starting wallet and capacity, four starters, the Level 2 Pearl, and computed catalog and XP outputs are mapped. Role, rarity, schedule, and premium weights influence the imported workbook results. | **Partial.** The six-minute first session at B10 is not demonstrated. The recurring sink target at B83 has no runtime mechanism. Legacy rows marked unused should not be added back to the game. Tuned formulas require recalculation and re-import before runtime values change. |
| **XP Curve** | Proposed cumulative XP at G4:G43 drives Levels 1 to 40. Level 2 requires 80 XP; Level 40 requires 996,160 XP. | **Launch curve implemented.** Levels 41 to 70 are stored as future values, not reachable levels. Unlock descriptions in H contain other features that are not delivered simply by importing XP. |
| **Coin Fish** | All 26 species at A2:AH27 are in the catalog. Names, levels, prices, stage times, care intervals, purchase XP, and four sale stages are mapped. Purchase and sale behavior is tested. | **Core catalog implemented.** Collection, decor synergy, and aquarium happiness language in AB18, AB23:AB25 is not a separate working mechanic. Biology fields AF:AG remain reference data. These descriptions do not define enough rules to implement those secondary systems literally. |
| **Premium Fish** | All 10 species at A2:AA11 have their prices, level gates, timing, and stage rewards. Bubble Eye is repeatable, following AA2. | **Partial as a player system.** Ongoing Pearl acquisition is absent. Unique-per-fish Koi patterns and timed Flashlight Fish glow described in X3 and X7 are not implemented as distinct rendering behavior. Species art exists, with shared fish animation. |
| **Limited Edition** | All 10 entries are imported. Nine explicit date windows work, including Frost Angelfish across New Year. Configurable annual, free, non-resellable claim support exists. | **Partial.** Anniversary Rainbowfish at D11 has no dates and is unavailable. Seven of the nine dated species require Pearls the player cannot accumulate. Young and Mature sale values are derived because this sheet only supplies Junior and Adult values. The derivations are recorded and checked. |
| **Unlock Plan** | The starter roster at C4, catalog fish gates, and the five launch tank levels are enforced. | **Partial.** Reward crates, the seven rotating decor themes in H4:H43, and social beats in I8/I15/I21/I30/I39 have no complete implementation. Medium/Large beats in G are retained, but their mapping to the four capacity steps is unresolved. A45 explicitly limits launch to Level 40. |
| **Tanks & Social** | Tank 1 to 5 levels and all four coin and token costs at B4:H8 are mapped. Ownership order, capacity limits, purchases, and token deduction work. | **Partial and blocked in normal progression.** No token source or friend-assist route exists. Tanks 6 to 8 at A9:I11 are future content. The current interpretation is coins plus either the solo material route or a future friend route. |
| **Onboarding** | The initial fish and wallet exist. General fish details, feeding, buying, placement, selling, and decor controls exist independently. Some unused tutorial code remains. | **Not delivered as a tutorial.** The 12-step flow at A4:G15 is absent from ordinary play. The current tutorial has 11 event states, no starter-tapping stage, no control gates, and an empty reward configuration. See the step audit below. |
| **Quests & Live Ops** | Four daily objectives have targets, level gates, progress, reset, and exact XP. Species mastery counts Adults and saves three badge claims. Dated limited windows work. | **Partial.** Daily coins, the Tank Stylist score reward, both weekly quests, mastery XP/statues, Decorator achievements, and the Daily Egg Basket are incomplete. |
| **Currency Matrix** | Coins, Pearls, XP, tank slots, and Gift Token balances exist. Fish purchases and sales, decor purchases, tank costs, and limited Pearl spending work. | **Partial.** Many sources and sinks in C4:D10 are missing. Event Eggs and a functioning Decor Score system are absent. The no-bankruptcy and solo-viability guardrails are not met by current play. |
| **Dashboard** | The workbook's metrics and formula outputs are retained as balancing references. Catalog counts match the game. | **Design tool, not an in-game feature.** Pacing and income targets are not measured player results. Its count of 10 limited entries includes the unscheduled anniversary entry. A reported PASS does not prove a working gameplay path. |
| **Roadmap** | All 53 proposed species and their five waves are retained in raw source data. | **Future work by design.** None are activated in the 46-species runtime catalog. Wave 1 has 21 species, Wave 2 has 7, Wave 3 has 10, Wave 4 has 5, and Wave 5 has 10. Suggested launch-level fish are still labeled post-launch content. |
| **Sources** | The 17 source references at A4:D20 are retained. | **Reference register.** No player implementation is required. External websites were not revalidated; this audit compares the supplied workbook with this project. |
| **Economy Simulation** | The 70-level approximation and its formulas are retained. | **Design model, not runtime verification.** The model assumes capacity from level unlocks without simulating the missing token earning route or purchase decisions. Its 251-day Level 70 estimate cannot describe the current Level 40 game with blocked capacity. |
| **Quest Scaling** | Exact D:G daily XP values for Levels 2 to 40 are mapped and granted. H weekly XP and I estimated coin pools are imported. | **Partial.** Weekly XP is never claimable; estimated coin pools are never paid. Levels 41 to 70 remain future data. The Level 40 profile still references the workbook's Level 41 delta even though gameplay caps at 40. |
| **Balance Review** | Core payout, premium conversion, stage XP, sickness, and rarity changes are reflected in the imported values. | **Workbook checks only.** PASS labels do not cover missing grants, social flows, recovery, or ordinary-play access. B6 still calls Bubble Eye one-time, conflicting with the detailed premium note. |

## Quest, mastery, and live-ops details

| Source row | What currently works | What still needs implementation or a decision |
|---|---|---|
| Quests & Live Ops 4: Feed Caretaker | Level 2 gate; eight healthy feed actions; incomplete progress resets when a living, unstashed fish becomes sick; claimed/completed progress and daily reset work. | Coin reward is absent. The workbook says “8 fish”; implementation counts feed actions, including repeated feeds of one fish. Confirm whether eight distinct fish are intended. |
| Row 5: Junior Seller | Level 2 gate; two sales of living, resellable Junior-or-older fish; exact XP; daily reset. | Coin reward is absent. Selling a gifted non-resellable species remains prohibited. |
| Row 6: Adult Harvest | Level 3 gate; one Adult sale; exact XP; daily reset. | Coin reward is absent. |
| Row 7: Tank Stylist | Level 3 gate; one decor purchase-and-placement transaction; exact quest XP. | No quest score reward. The ordinary decor purchase also grants no XP and adds no score total. |
| Row 8: Collection Chapter | Level 8 gate and a five-entry target are stored. Weekly XP is known for each launch level. Unique species discovery exists. | No themed chapter membership, weekly chapter selection, usable completion flow, Pearl/event-egg reward, or enabled claim. A lifetime discovery counter is not a themed weekly collection. |
| Row 9: Neighbor Helper | Level 5 gate, weekly cadence, and five-gift target are stored. | Sending, receiving, NPC tickets, token rewards, and coin rewards are absent. The stored objective uses a gift event, but no working gift action emits it. |
| Row 10: Species Mastery | Adult growth increments once per fish. Targets are 5, 25, and 100 per species. Claims and badge display persist. Selling does not count as raising an Adult. | XP rewards and statue decorations are missing. Amounts, statue assets, placement rules, and whether statues are per tier or final-tier rewards need definition. |
| Row 11: Decorator | Local decor items carry score numbers in their definitions. | No score aggregation, achievement definition, threshold detection, title reward, background reward, or corresponding screen. Local item scores are not evidence of a functioning score system. |
| Row 12: Daily Egg Basket | The Level 6 gate and an unavailable action exist. | No random egg claim, rolling 24-hour timer, weekly table rotation, probabilities, retired pool, or resale protection. Define these before implementing grants. |
| Row 13: Limited Fish Window | Nine source date windows and purchase gates work. | Define anniversary dates and provide the intended Pearl access. Seasonal quest rewards and event egg crafting listed elsewhere are separate missing systems. |

Daily and weekly quest resets use UTC days and Monday-based weeks. That is the current implementation choice. It does not establish the rolling 24-hour behavior requested for the inactive Egg Basket.

At Level 10, a direct runtime check paid 72, 81, 92, and 44 XP for the four daily quests, matching `Quest Scaling!D12:G12`. All four paid zero coins, Pearls, and Gift Tokens. The workbook's estimated daily coin pool at I12 is 909, but there is no rule assigning that amount across claims. At Level 30, the weekly XP value of 18,788 in H32 is imported but unavailable to the player.

Evidence: [quest and mastery actions](/Users/amoghrijal/amogh/aquarium-native/src/domain.cpp:213), [quest configuration and claims](/Users/amoghrijal/amogh/aquarium-native/src/domain.cpp:230), [quest and gift screens](/Users/amoghrijal/amogh/aquarium-native/src/view.cpp:56), [mastery display](/Users/amoghrijal/amogh/aquarium-native/src/skin.cpp:398), [runtime checks](/Users/amoghrijal/amogh/aquarium-native/evidence/workbook-depth-audit-2026-09-11/runtime-probes.json).

## Onboarding step audit

Earlier project notes explicitly record tutorial work as deferred. That explains the gap but does not make the spreadsheet flow complete.

| Step and source | Current state |
|---|---|
| 1, Onboarding row 4: choose background and gravel | Four fish appear immediately. A stored look setting can tint the reef, but there is no active first-run choice, separate gravel selection, or store gate. |
| 2, row 5: tap each starter fish, earn 20 XP | Fish details work. There is no per-starter guided tapping sequence, one-at-a-time highlight, or 20 XP tutorial grant. |
| 3, row 6: feed all fish, earn 25 XP | Normal feeding works. The Feed-only gate and 25 XP tutorial grant are absent. |
| 4, row 7: buy Neon Tetra | Normal purchases give the correct purchase XP. No tutorial forces the specified fish or enables only its card. |
| 5, row 8: place fish, earn 10 XP | Placement works. The tutorial slot highlight and extra 10 XP are absent. |
| 6, row 9: free instant-grow demo | Dormant domain code can promote a fish to Junior once. The tutorial control is not available in normal play. |
| 7, row 10: sell Junior | Normal Junior selling and reward feedback work. The guided stage is absent. |
| 8, row 11: buy and place plant, earn 15 XP plus score | Placement works. Tutorial XP, score progression, and the single-card gate are absent. |
| 9, row 12: Level 2 reward | Reaching 80 XP automatically gives one Pearl and unlocks fish. There is a level toast, not the specified reward claim modal. A coin reward is mentioned but not quantified. |
| 10, row 13: first quest | The daily quest panel and XP claims work once unlocked. The tutorial direction, panel pulse, and coin payout are absent. |
| 11, row 14: free gift with NPC fallback | Gifts are unavailable; no NPC fallback or Gift Box item is granted. |
| 12, row 15: deadline/reminder and starter egg after 24 hours | Fish details show care timing. There is no scheduled reminder or 24-hour starter egg. The Notifications toggle only changes a view-local boolean and is not saved or connected to OS notifications. |

The 20, 25, 10, and 15 XP awards are explicit source requirements, totaling 70 XP. They can be implemented without inventing amounts. Other tutorial rewards, including the Level 2 coins and exact return egg, still need definition. The six-minute session target needs a complete-flow check once the tutorial is enabled.

Evidence: [inactive tutorial view](/Users/amoghrijal/amogh/aquarium-native/src/view.cpp:88), [tutorial flag](/Users/amoghrijal/amogh/aquarium-native/include/aquarium/view.hpp:98), [tutorial actions](/Users/amoghrijal/amogh/aquarium-native/src/domain.cpp:258), [notification toggle](/Users/amoghrijal/amogh/aquarium-native/src/skin.cpp:465).

## Currency and progression gaps

| Resource | Working sources and uses | Missing source requirements |
|---|---|---|
| Coins | Fish sale rewards; spending on coin fish, local decor, and configured tank costs. | Quest coin grants, daily bonuses, friend visits, achievement/collection payouts, basic food costs, and recurring maintenance. Food is explicitly free in the current game. Prices, cadence, and recovery safeguards need definition before adding sinks. |
| Pearls | One grant when first reaching Level 2; spending on premium fish, seasonal fish, and revival. | Further level grants, event grants, IAP, offers, premium gifts, event eggs, and normal paid speed-ups. Only the Level 2 amount is specified numerically in this workbook. |
| XP | Fish purchase XP, stage sale XP, and four daily quest XP rewards. | Ordinary decor purchase XP, tutorial XP, weekly collection XP, mastery XP, and social/collection grants. |
| Tank slots | Starts at 10. Upgrades and purchases correctly charge coins plus tokens when a balance is supplied. | A usable source of tokens, friend assists, defined expansion-level mapping, and optional discounts. |
| Gift Tokens | Balance storage and tank deductions. Existing save balances remain usable. | Daily gifts, NPC helper grants, Neighbor Helper rewards, social fish, event crafting, and the fuller discount model. |
| Decor Score | Five local item definitions have score values. Decor placements are saved. | Total calculation and display, level-gated themes, threshold rewards, showcases, contests, titles, and backgrounds. |
| Event Eggs | Purchased fish have normal egg hatching. | Separate collectible egg inventory, Basket rewards, event quest rewards, gift chains, crafting, retired fish drops, and related resale rules. Ordinary purchase eggs do not implement this resource. |

The tank gap was checked at Level 7 with 1,000,000 coins and zero tokens. Expansion still required 250 coins plus five tokens; Tank 2 required 2,500 coins plus ten tokens. Both failed, and gifting was unavailable. The existing tests that prove tank purchases use preloaded token balances, so their success does not prove a new player can acquire a tank.

The bankruptcy case used ordinary purchases: three Molly and one Neon Tetra consume 247 coins, then one Bubble Eye uses the earned Level 2 Pearl. After seven days without feeding, all fish are dead. The save has three coins and zero Pearls, no buyable species, and revival reports insufficient currency. This conflicts with the recovery intent in `Currency Matrix!E4`; the exact rescue policy is not supplied. This is an early-game reachability check, not a measured first-session pacing test.

Evidence: [purchase, feeding, revival, and decor actions](/Users/amoghrijal/amogh/aquarium-native/src/domain.cpp:153), [tank transactions](/Users/amoghrijal/amogh/aquarium-native/src/domain.cpp:188), [Level 2 grant](/Users/amoghrijal/amogh/aquarium-native/src/domain.cpp:86), [runtime checks](/Users/amoghrijal/amogh/aquarium-native/evidence/workbook-depth-audit-2026-09-11/runtime-probes.json).

## Source decisions and incomplete specifications

1. **Bubble Eye purchase policy conflicts within the workbook.** `Premium Fish!AA2` explicitly says repeatable. `Unlock Plan!F5`, `Inputs!F111`, `XP Curve!H5`, and `Balance Review!B6` call it one-time. The importer deliberately follows AA2. Current behavior is consistent with that recorded decision; this is not an accidental missing limit.
2. **Tank expansion gates are incomplete.** There are four capacities, 10/20/30/40, but Medium/Large beats do not clearly assign a level to every step. Runtime enforces tank ownership and unlock levels but not those per-step beats. Resolve the mapping before claiming level-gated capacity parity.
3. **Daily coin pools are estimates, not per-quest reward assignments.** The pool is calculated, but the individual split is missing. Assigning the entire pool to each quest would overpay it.
4. **Mastery and Decorator reward details are incomplete.** Targets for mastery are known; XP amounts and statue definitions are not. Decorator has neither score thresholds nor a reward catalog.
5. **Collections need membership and completion rules.** Define themes, species lists, weekly selection, how previously discovered fish count, and whether the Pearl/event-egg reward is fixed or chosen.
6. **The Egg Basket needs a table and timing contract.** Define weights, weekly rotation, the retired pool, the 24-hour rule, and resale handling. Define the tutorial return egg separately.
7. **Social progression needs amounts as well as flows.** Define sending, receiving, NPC tickets, daily limits, assist counting, and token grants. The workbook also gives different social beats: daily gifting at Level 5 and friend assist/NPC helper at Level 12. The onboarding NPC fallback must be reconciled with these gates.
8. **Anniversary Rainbowfish needs its annual dates.** Do not infer dates from “1 week, yearly.”
9. **Several numerical choices are local assumptions.** The five decor prices/scores, free food, the six-second hatch, hunger/growth-pause thresholds, and one-Pearl revival are not all numerically defined by the workbook. Existing behavior should be documented and deliberately accepted or replaced.
10. **Descriptive fish traits are not complete feature definitions.** Decor synergy, aquarium happiness, breeding fantasy, and prestige language identify design intent but do not define a breeding or happiness system. The two concrete premium art behaviors noted above are separate remaining visual work.

## Future content is separate from launch gaps

The launch cap of Level 40 is explicit in `Unlock Plan!A45`. The 30 later XP values, Tanks 6 to 8, and all 53 Roadmap entries are planned future work. They should not inflate a count of missing launch features.

Activating these later requires more than moving data between sheets. The content loader requires exactly 46 species, the XP array has 40 entries, tank arrays have five entries, UI tank lists have five choices, and save validation also enforces those limits. Import checks and tests encode the same launch assumptions. Those limits must change together when a wave ships.

Within the launch unlock plan, levels without fish are described as “Reward crate + roadmap fish slot.” The Roadmap fish are intentionally future, but there is no working reward crate substitute today. Crate contents and grants need specification if these levels are expected to deliver a reward at launch.

Evidence: [runtime limits](/Users/amoghrijal/amogh/aquarium-native/include/aquarium/domain.hpp:31), [import scope](/Users/amoghrijal/amogh/aquarium-native/tools/import_content.py:256), [save validation](/Users/amoghrijal/amogh/aquarium-native/src/storage.cpp:31), [tank list](/Users/amoghrijal/amogh/aquarium-native/src/skin.cpp:252).

## Validation performed

| Check | Current result | What it establishes |
|---|---|---|
| All-sheet extraction | 17 worksheets, 589 populated rows; no cached Excel error values found. | Source coverage. Formula text and cached outputs were read without editing or recalculating the workbook. |
| Workbook identity | Imported SHA-256 matches the source file. | The content records this workbook revision. |
| Independent catalog comparison | 1,672 comparisons passed: 1,439 explicit fields and 233 derived fields. | Mapped content matches workbook values. This is not whole-feature completeness. |
| Importer regression tests | 3 of 3 passed. | Deterministic rebuilding, preservation of unmapped fields, and atomic failure behavior. |
| Current desktop build | Build succeeded; existing objects were current. | The tested desktop binaries were current for the checked tree. |
| Domain and storage tests | 29 of 29 passed. | Catalog actions, care timing, stage sales, quest XP, mastery, tanks with supplied balances, event rules, and saves. |
| UI, fish details, renderer, and menu motion | All four suites passed with SDL dummy video and software rendering. | Automated interaction/render checks pass without a display. |
| Default native-display test attempt | The four UI suites could not initialize a display in this session. | Environment limitation, not an observed product assertion failure. It was resolved for automated checks by the headless run. |
| Additional runtime probes | Confirmed token blockers, the one-Pearl limit, XP-only daily rewards, unavailable weekly claims, no decor purchase XP, and the bankruptcy case. | Specific gaps reproduced against current domain code. Tests with manually supplied level/progress are used only to isolate gates; the bankruptcy sequence uses normal purchases. |

This audit does not certify physical-device rendering, notification delivery, IAP, a live backend, or actual retention and time-to-level results. No new screenshots were manually rated for art fidelity. Passing renderer tests does not verify the premium art direction described in the workbook.

Saved evidence: [catalog result](/Users/amoghrijal/amogh/aquarium-native/evidence/workbook-depth-audit-2026-09-11/catalog.json), [workbook summary and domain results](/Users/amoghrijal/amogh/aquarium-native/evidence/workbook-depth-audit-2026-09-11/workbook-summary.json), [importer tests](/Users/amoghrijal/amogh/aquarium-native/evidence/workbook-depth-audit-2026-09-11/import-tests.log), [software rendering tests](/Users/amoghrijal/amogh/aquarium-native/evidence/workbook-depth-audit-2026-09-11/ctest-headless.log), [runtime checks](/Users/amoghrijal/amogh/aquarium-native/evidence/workbook-depth-audit-2026-09-11/runtime-probes.json).

## Suggested implementation order

1. Define and implement the solo token earning route and the all-fish-lost recovery rule. Verify that a new save can expand and recover without a supplied balance.
2. Define ongoing Pearl grants and finish the first-session tutorial, including the explicit XP rewards and return flow.
3. Complete daily reward rules, themed weekly collections, and mastery XP/statues. Add ordinary-play checks for earning and claiming each reward.
4. Implement decor score, XP, themes, thresholds, and cosmetic rewards.
5. Build the Daily Egg Basket, anniversary scheduling, and social/NPC gift flows. Complete the friend-assist alternative and event egg resource.
6. Add agreed recurring coin sinks and remeasure pacing using the actual earning and expansion paths. Keep the workbook simulation distinct from observed play.
7. Deliver roadmap waves and Levels 41 to 70 as separate post-launch changes.

The earlier feature matrix remains useful as history, but its labels for “five tank schedules,” “decor score,” and “four daily quests” need the qualifications above. Accurate catalog data and passing tests are substantial progress; they do not yet provide the complete progression experience described by the spreadsheet.
