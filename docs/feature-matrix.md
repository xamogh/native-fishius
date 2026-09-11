# Feature completion matrix

This matrix separates code that is implemented, checks that have actually run, and work the user deferred. It does not claim that every workbook idea has enough detail to ship. See `workbook-implementation.md` for source decisions and `assumptions.md` for the remaining local values.

The current recorded checks are `evidence/workbook-catalog-check.log` and `evidence/workbook-domain-tests.json`. Earlier UI screenshots and test logs under `evidence/aquarium-match` belong to the earlier layout work; they are not verification of this content update.

| Requirement | Owning module | Status | Evidence or remaining work |
|---|---|---|---|
| Native C++20, SDL3 renderer and text | CMake, Canvas, main | Implemented | Existing desktop and simulator build paths; use the logs for the build being reviewed |
| Exact 46-fish roster and staged economy | Import tools, Content | Implemented and checked | 26 coin, 10 premium and 10 limited fish; independent workbook run passed 1,672 comparisons |
| Launch XP curve, starting wallet and Level 2 Pearl | Content, Domain | Implemented and checked | Imported launch levels 1 to 40 and exact values; later curve values retained as future content |
| Hungry Baby Neon Tetra, Guppy, Platy and Molly | Content, Domain | Implemented and checked | Starter test passes; existing saves are not reset |
| Six-second hatch, growth and care deadlines | Domain | Implemented and checked | Boundary tests and large versus small advances pass |
| Buy arming, placement and cancellation | View, Domain | Implemented | Transaction checks pass; this update needs its own UI pass |
| Purchase and sale atomicity | Domain | Implemented and checked | All species and staged rewards, rejection and overflow cases covered |
| Physical food and first-arrival consumption | Domain, Canvas | Implemented and checked | One pellet feeds one fish after its notice delay; bounded-food test passes |
| Cruising, dashes, turns and pursuit | Domain, Canvas | Implemented | Existing movement path; no new mobile frame-rate claim |
| Fish wave animation and following outlines | Canvas | Implemented | Appearance and phase continuity require visual review after asset integration |
| Select, move, stash and restore | View, Domain | Implemented | Identity and paused timer checks pass; current gestures need UI review |
| Sell assistance and sale feedback | View, Canvas, Domain | Implemented | Domain sale checks pass; animation review is separate |
| Sickness, corpses and revival | Domain, Canvas | Implemented and checked | Boundaries, bulk actions and capacity overflow pass |
| Five workbook tank and capacity schedules | Content, Domain, View | Implemented and checked | Exact costs and Gift Tokens; tank unlock levels 1, 7, 16, 25 and 34 |
| Solo tank purchase interpretation | Domain | Implemented | Coins plus Gift Tokens; friend assists unavailable; capacity-to-expansion-level mapping remains unconfirmed |
| Shop catalog, filters and purchase blockers | View | Implemented | Uses real groups and unlock order, level/care/price labels and domain restrictions; new mobile review pending |
| Full species discovery collection | View, Domain | Implemented | All 46 species can be tracked; themed chapters remain deferred |
| Adult mastery at 5, 25 and 100 | Content, Domain, Storage, View | Implemented and checked | Adult growth counts once; persistent tier claims and old-save migration pass |
| Species-specific canonical artwork | Canvas, species assets | Runtime integration implemented | Six existing and 40 generated sprites; canonical alpha crop, left normalization, 512-pixel limit and runtime mask/dead variants; final asset and visual checks tracked separately |
| Decor purchase, placement and score | Domain, View | Implemented with local values | Workbook lacks individual decor prices and scores; defaults listed in `assumptions.md` |
| Four daily quests | Import tools, Domain, View | Implemented and checked | Exact XP columns, targets and level gates; no invented split of the daily coin pool |
| Weekly collection and Neighbor Helper | Content, Domain, View | Deferred | Definitions and known gates retained; missing themes, gift rules and rewards keep claims unavailable |
| Tutorial | Domain, View | Deferred by user | Start guide entry removed; unused code and saved state retained; no new completion or reward claim |
| Mastery XP/statues and decorator rewards | Content, Domain | Deferred | Missing reward amounts, thresholds and asset specifications are recorded |
| Gift Tokens and Daily Egg Basket | Domain, View | Deferred | Existing balances/history retained; invented NPC token and egg grants removed |
| Nine dated limited events | Import tools, Domain | Implemented and checked | Source dates, year-crossing ranges and annual claim rules covered |
| Anniversary Rainbowfish event | Content, Domain | Deferred by user | No source dates; unavailable until configured |
| Saves, recovery and offline catch-up | Storage, Session | Implemented and checked | Round-trip, corrupt-primary preservation, backup and suspension tests pass; packaged lifecycle review is separate |
| Compatibility with older native saves | Storage | Implemented and checked | Sales history is retained; new Adult mastery starts empty; only unclaimed old healthy-feed progress resets |
| Reduced motion and sound controls | View, Canvas | Existing partial support | Ambient audio, full motion suppression and interruptions need review |
| XP bar animation detail | View | Existing presentation | Exact reference timing is not certified |
| Small phone targets and safe areas | View, Canvas | Existing adaptive layout | Earlier mobile layout work is recorded; content update requires a fresh visual and interaction pass |
| Renderer resource-loss recovery | SDL host | Existing lifecycle handling | Full texture recreation after renderer reset still needs review |
| Android and iOS hosts | platform | Build configuration exists | iOS Simulator was used for earlier UI work; no current physical-device or Android certification |
| Reference image parity | Evidence tools | Earlier layout review recorded | Workbook-correct content intentionally replaces screenshot-specific roster and ordering |
| Mobile performance target | Evidence tools | Not certified | Native-host measurements do not establish mobile 60 FPS |
