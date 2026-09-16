# Feature completion matrix

Current economy and lifecycle rules are in [v4 implementation, steps 1 to 3](workbook-implementation.md). That record supersedes older prices, purchase XP, care penalties, capacity and save compatibility statements below.

This matrix separates code that is implemented, checks that have actually run, and work the user deferred. It does not claim that every workbook idea has enough detail to ship. See `workbook-implementation.md` for source decisions and `assumptions.md` for the remaining local values.

The 12 September audit and follow-up fixes are recorded in `workbook-audit-2026-09-12.md` and `workbook-fixes-2026-09-12.md`. Earlier fish-integration checks are `evidence/workbook-catalog-check.log` and `evidence/workbook-domain-tests.json`. Earlier UI screenshots and test logs under `evidence/aquarium-match` belong to the earlier layout work; they are not verification of this content update.

| Requirement | Owning module | Status | Evidence or remaining work |
|---|---|---|---|
| Native C++20, SDL3 renderer and text | CMake, Canvas, main | Implemented | Existing desktop and simulator build paths; use the logs for the build being reviewed |
| Exact 46-fish roster and staged economy | Import tools, Content | Implemented and checked | 26 coin, 10 premium and 10 limited fish; independent workbook run passed 1,672 comparisons |
| Launch XP curve, starting wallet and Level 2 Pearl | Content, Domain | Implemented and checked | Imported launch levels 1 to 40 and exact values; later curve values retained as future content |
| Level-up screen and rewards at levels 2 to 40 | View, Domain, Content | Implemented and checked | User-directed reward schedule: 50 times the reached level in coins and one pearl; real catalog unlocks, paging, modal controls and duplicate prevention; see `level-rewards.md` and `evidence/level-up/` |
| Hungry Baby Neon Tetra, Guppy, Platy and Molly | Content, Domain | Implemented and checked | Starter test passes; existing saves are not reset |
| Six-second hatch, growth and care deadlines | Domain | Implemented and checked | Boundary tests and large versus small advances pass |
| Egg placement and cancellation | View, Domain, Storage | Implemented | FishX flow: choose a species for free, buy and place one egg per primary tap, repeat until Done or a blocker; old paid eggs recover into Bag |
| Purchase and sale atomicity | Domain | Implemented and checked | All species and staged rewards, rejection and overflow cases covered |
| Physical food and first-arrival consumption | Domain, Canvas | Implemented and checked | One pellet feeds one fish after its notice delay; bounded-food test passes |
| Cruising, dashes, turns and pursuit | Domain, Canvas | Implemented | Existing movement path; no new mobile frame-rate claim |
| Fish wave animation and following outlines | Canvas | Implemented | Appearance and phase continuity require visual review after asset integration |
| Select, move, stash and restore | View, Domain | Implemented | Fish selection remains available; decor editing requires Arrange tank; stored copies keep identity and can transfer for free |
| Sell assistance and sale feedback | View, Canvas, Domain | Implemented | Domain sale checks pass; animation review is separate |
| Sickness, corpses and revival | Domain, Canvas | Implemented and checked | Boundaries, bulk actions and capacity overflow pass |
| Five workbook tank and capacity schedules | Content, Domain, View | Implemented and checked | Exact Coin and Pearl costs; tank unlock levels 1, 7, 16, 25 and 34 |
| Tank purchase interpretation | Domain | Implemented | Coins or Pearls, chosen by the price button; friend assists unavailable; capacity-to-expansion-level mapping remains unconfirmed |
| Shop catalog, filters and purchase blockers | View | Implemented | Unlock ordering and domain restrictions retained; limited fish and decor appear during active configured events; unconfigured events remain closed |
| Full species discovery collection | View, Domain | Implemented | All 46 species can be tracked; themed chapters remain deferred |
| Adult mastery at 5, 25 and 100 | Content, Domain, Storage, View | Implemented and checked | Adult growth counts once; persistent tier claims and old-save migration pass |
| Species-specific canonical artwork | Canvas, species assets | Runtime integration implemented | Six existing and 40 generated sprites, canonical derivatives, stable per-fish Koi patterns and timed Flashlight Fish cheek glow; visual limits remain recorded separately |
| Decor purchase, placement and score | Domain, View | Workbook catalog implemented | 120 items, permanent paid copies, first-ownership XP, distinct placed-ID score, free storage/transfers, visible-footprint bounds and workbook layer order |
| Four daily quests | Import tools, Domain, View | XP and objectives implemented | Exact XP columns, targets and level gates; Tank Stylist requires placement or a real rearrangement; daily coin split remains undefined |
| Weekly collection and Neighbor Helper | Content, Domain, View | Deferred | Definitions and known gates retained; missing themes, gift rules and rewards keep claims unavailable |
| Tutorial | Domain, View | Deferred by user | Start guide entry removed; unused code and saved state retained; no new completion or reward claim |
| Mastery XP/statues and decorator rewards | Content, Domain | Deferred | Missing reward amounts, thresholds and asset specifications are recorded |
| Gifting and Daily Egg Basket | Domain, View | Deferred | Gift and Basket rules remain undefined; tank progression uses Pearls instead |
| Early bankruptcy recovery and other currency sources | Domain, Content | Partly undefined | Every level from 2 to 40 now grants one pearl and coins. Rescue eligibility and payment integration remain undefined. |
| Nine dated limited events | Import tools, Domain, View | Implemented | Source dates, year-crossing ranges and annual claim rules retained; Shop exposes active event fish |
| Anniversary Rainbowfish event | Content, Domain | Deferred by user | No source dates; unavailable until configured |
| Saves, recovery and offline catch-up | Storage, Session, View | Implemented | Persistent save-failure feedback, retry and recovery notices added; final checkpoint failure is reported; packaged lifecycle review remains separate |
| Compatibility with older native saves | Storage | Implemented and checked | Sales history is retained; new Adult mastery starts empty; only unclaimed old healthy-feed progress resets |
| Reduced motion and sound controls | View, Canvas | Persisted controls implemented | Independent Reduced Motion switch and sound slider read saved state; music is marked unavailable; ambient audio and interruption handling remain separate work |
| XP bar animation detail | View | Existing presentation | Exact reference timing is not certified |
| Small phone targets and safe areas | View, Canvas | Existing adaptive layout | Earlier mobile layout work is recorded; content update requires a fresh visual and interaction pass |
| Renderer resource-loss recovery | SDL host | Existing lifecycle handling | Full texture recreation after renderer reset still needs review |
| Android and iOS hosts | platform | Build configuration exists | iOS Simulator was used for earlier UI work; no current physical-device or Android certification |
| Reference image parity | Evidence tools | Earlier layout review recorded | Workbook-correct content intentionally replaces screenshot-specific roster and ordering |
| Mobile performance target | Evidence tools | Not certified | Native-host measurements do not establish mobile 60 FPS |
