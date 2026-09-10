# Feature completion matrix

Status terms: **implemented in source** means code exists; **executed** requires a successful command in the evidence report; **unverified** means it has not been established by the available evidence. This matrix is not a claim that the full requested game is complete.

| Requirement | Owning module | Source implementation | Verification or remaining gap |
|---|---|---|---|
| Native C++20, SDL3 renderer and text | CMake, Canvas, main | Implemented in source | See actual configuration and compiler logs |
| 46 catalog definitions and exact staged economy | Import tools, Content | Import and validation paths implemented | Independent workbook-cell mapping still needs review |
| Initial state and hungry babies | Domain | Implemented in source | Domain tests provided |
| Six-second hatch, growth and care deadlines | Domain | Implemented in source | Boundary and large/small advance tests provided |
| Buy arming, repeated placement, cancellation | View, Domain | Implemented in source | Full pointer-driven integration verification outstanding |
| Purchase and sale atomicity | Domain | Implemented in source | Transaction and duplicate-ID tests provided |
| Physical food and first-arrival consumption | Domain, Canvas | Implemented in source | Domain tie and food-bound tests provided; visual timing review outstanding |
| Cruising, dashes, animated turns, pursuit | Domain | Implemented in source | Clips only count as evidence if actually captured |
| Traveling mesh wave and following outlines | Canvas | Implemented in source | Visual and phase-continuity review outstanding |
| Select, move, stash, restore | View, Domain | Implemented in source | Domain pause tests provided; gesture integration review outstanding |
| Net aim assistance and sale feedback | View, Canvas, Domain | Implemented in source | Actual interaction and motion review outstanding |
| Sickness, desaturated corpses and revival | Domain, Canvas | Implemented in source | Care, bulk action and capacity overflow tests provided |
| Five tanks and explicit upgrade schedule | Domain, View | Implemented in source | Price, gate and maximum-step tests provided |
| Shop, inventory, collection and settings panels | View | Implemented in source | Layout captures may be generated; exact reference comparison not certified |
| Decor purchase, placement and score | Domain, View | Local configuration implemented | Numeric defaults are documented assumptions |
| Daily quests and weekly goals | Domain, View, importer | Event counters, resets and claims implemented | Missing reward mappings block some claims; not complete until resolved |
| Tutorial | Domain, View, importer | Saved steps and once-only paths implemented | Reward mapping and full walkthrough outstanding |
| Collections, mastery and achievements | Domain, View | Discovery and local mastery badges implemented | Full workbook collection chapters and achievement milestones not established |
| Gift Tokens, solo NPC and daily eggs | Domain, View | Local path implemented | Some values and rules are documented assumptions |
| Limited and annual events | Domain, importer | Date and claim gates implemented | Exact imported dates need source verification |
| Saves, corrupt-primary preservation, offline catch-up | Storage, Session | Implemented in source | Domain/storage tests provided; packaged relaunch and mobile lifecycle not certified |
| Reduced motion and sound controls | View, Canvas | Partial presentation controls implemented | Ambient audio, all motion suppression, and interruptions require review |
| XP bar transition and reference animation detail | View | Basic presentation implemented | Exact 300-millisecond XP transition not established |
| Small phone targets and aspect adaptation | View, Canvas | Adaptive layouts implemented | Native safe areas and all minimum targets not verified |
| Renderer resource-loss recovery | SDL host | Lifecycle handling present | Full texture recreation after renderer reset outstanding |
| Android and iOS hosts | platform | Configuration and host glue provided | SDK compilation, signing, installation and device verification outstanding |
| Reference image parity | Evidence tools | Four comparison states supported | Synthetic fixtures are not exact captured screenshot state |
| Performance target | Evidence tools | Native measurement path provided | No mobile 60-FPS claim; use actual report if produced |
