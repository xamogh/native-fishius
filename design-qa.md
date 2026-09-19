# Decoration alignment across devices, 18 September 2026

final result: passed

Backgrounds and decor now share one scene projection, including sprite size, drag coordinates, selection glow and purchase feedback. The same saved layout stays on the same scenery across phone and tablet viewports. Resizing does not modify the save. Scene edges and attached objects crop together, and new drops fit the visible crop.

Both backgrounds were visually checked at phone and tablet sizes. The desktop and iOS simulator builds and six relevant test suites passed. The [review and captures](evidence/placement-alignment-2026-09-18/design-qa.md) record the regression, input and save checks, native captures and device-testing limits. Earlier reviews follow.

# Selected aquarium backgrounds, 18 September 2026

final result: passed

Implemented the approved second and third concepts as Open Blue Cove and Sage Lagoon, with clean background artwork and the original selected colors. Saved background IDs, unlocks and prices carry over. The earlier runtime muting filter is removed.

The [blue cove comparison](evidence/backgrounds-2026-09-18/comparison-blue-cove.png) and [sage lagoon comparison](evidence/backgrounds-2026-09-18/comparison-sage-lagoon.png) were reviewed against native captures at the same scene size and state. Desktop, phone and tablet views and the Backgrounds shop were checked. The desktop build and all five relevant suites passed. See the [full review](evidence/backgrounds-2026-09-18/design-qa.md) for the five fidelity surfaces, exact sources, captures, test coverage and limits. No physical-device or simulator check was performed. Earlier reviews follow.

# Favorites and shared fish capacity, 18 September 2026

final result: passed

Removed the Keep action and its reward conversion. Fish stay until sold, and the heart protects favorites from sale. The fish panel has one Sell button; fish without sale rewards offer Feed fish. All eggs and fish share normal tank capacity. The landscape adult panel and portrait favorite panel were reviewed in `evidence/fish-favorites-2026-09-18`; care interactions pass at six viewport sizes.

The source workbook now follows the same rules. All 39 sheets, existing prices, rewards, formulas other than the stale-status wording, and formatting are preserved. The retired extra fish allowance is zero. Baseline simulation snapshots are marked stale and require a rerun for shared capacity. The desktop build, all 21 software-renderer regression suites, two importer tests and 2,333 catalog checks pass. No physical-device check was performed in this pass. Earlier reviews follow.

# Settings cards and amber controls, 18 September 2026

final result: passed

Menu follow-up: Removed How to play and its guide. About & support is now the
only footer button and is centered beneath the cards. The desktop build and
seven-viewport Settings suite passed. Landscape and portrait captures were
checked, along with the About & support page.

Font follow-up: Settings now uses the same Luckiest Guy font as Shop, fish
details and shared dialog controls. The [updated phone screen](evidence/settings-2026-09-18/fonts/settings-phone.png)
was checked against the current Shop in [the font comparison](evidence/settings-2026-09-18/fonts/font-comparison.png).
Both builds and the seven-viewport Settings suite passed again.

Implemented the approved four-card Settings screen with saved music, volume,
sound effects, reduced motion and vibration controls. How to play and About &
support open usable pages. The [approved reference](design/mockups/settings-2026-09-18/approved.png)
and [native phone screen](evidence/settings-2026-09-18/settings-phone.png) were
reviewed in the same [final comparison](evidence/settings-2026-09-18/comparison-final.png).
The palette, padding and typography were corrected after the first comparison.
Seven viewport layouts and interaction states pass. All 21 tests and both desktop
and iOS simulator builds pass. Physical-device haptics remain unverified. See
[the full review](evidence/settings-2026-09-18/design-qa.md) for evidence and limits.
Earlier reviews follow.

# Mobile-first level receipt, 18 September 2026

final result: passed

Implemented the selected first phone design: a fixed left receipt and Continue button, with square item cards that swipe horizontally on the right. The later copy request is included: currencies show only their icons and amounts. The shared frame, real credited rewards and 500 ms fly-to-HUD animation remain.

The [selected mock](design/mockups/level-up-2026-09-18/mobile-first/approved.png) and [final phone view](evidence/level-up-mobile-first-2026-09-18/final/level-40-844x390-top.png) were opened together in the [normalized comparison](evidence/level-up-mobile-first-2026-09-18/comparison-final.png). A [running iPhone simulator capture](evidence/level-up-mobile-first-2026-09-18/iphone-simulator.png) confirms native rendering and safe-area fit. Seven viewport sizes and both scroll ends were captured. Both builds and all eight relevant test suites pass. The [full review](evidence/level-up-mobile-first-2026-09-18/design-qa.md) records visual corrections, asset prompts, test coverage and verification limits. Earlier reviews follow.

# Level-up dialog, 18 September 2026

final result: passed

Implemented the approved first design, with “Added to your balance” and “Now available to buy” removed. The existing blue frame, wave header and red close control remain. The receipt shows real earned coins and pearls, pages through catalog unlocks, and dismisses without changing saved rewards.

The [selected reference](design/mockups/level-up-2026-09-18/approved.png) and [rendered implementation](evidence/level-up-2026-09-18/level-2-1440x840-page-1.png) were reviewed in the same [combined comparison](evidence/level-up-2026-09-18/comparison-final.png), normalized to 1440 by 840 pixels. Native, phone, portrait, safe-area and later-level captures were also checked. Typography, spacing, colors, image fidelity and copy pass. Both builds and the relevant domain, UI and native rendering checks pass. See the [full review](evidence/level-up-2026-09-18/design-qa.md) for comparison history and validation limits. Earlier reviews follow.

# Bottle error toast, 18 September 2026

final result: passed

The bottle toast is another 20 percent smaller and appears at the top right, just below Settings. It keeps the downward bounce, 3.5-second timeout and 44-point close target. Favourited fish, including kept fish, show a small heart only while Sell is active. The marker follows swimming and turns, clears the sell meter, and hides when Sell ends. Desktop and iOS simulator builds and native care checks pass. See the [visibility correction](evidence/bottle-toast/sell-only/design-qa.md) and [before and after views](evidence/bottle-toast/sell-only/comparison-full.png). The [toast and heart design review](evidence/bottle-toast/top-right/design-qa.md), [previous review](evidence/bottle-toast/compact/design-qa.md) and [original implementation review](evidence/bottle-toast/design-qa.md) retain the earlier iteration history. Earlier reviews follow.

# Live fish growth percentage, 17 September 2026

final result: passed

Growth now displays two decimal places and updates with the live simulation. The number area fits 100.00%. A growing Neon Tetra advances from 9.99% to 10.00% within the same open popover; paused growth stays fixed, and 99.99% reaches 100.00% only at completion.

Previous and updated phone views were reviewed together, followed by portrait, tablet and desktop captures. Both builds, care checks, HUD layout and rendering checks pass. No actionable P0/P1/P2 finding remains. See the [full review](evidence/fish-live-percent/design-qa.md). Earlier reviews follow.

# Fish unlock badge, 17 September 2026

final result: passed

The adult back/profit line is removed. Eggs and Babies show Unlocks at Junior in a lavender badge above the growth timer. The two badge rows have room between them at phone, portrait and desktop sizes. Existing button colors, padlocks and immediate selling are retained.

The previous and updated Baby views were reviewed together at 667 by 375 pixels, 1x native density. A favorite Molly fixture follows the supplied screenshot's 17% growth and adult reward values. Both builds, the care suite, HUD layout and rendering checks pass. No actionable P0/P1/P2 finding remains. See the [full review](evidence/fish-unlock-badge/design-qa.md) and [updated Molly](evidence/fish-unlock-badge/final/1672x941/molly-growing.png). Earlier reviews follow.

# Fish actions and growth badges, 17 September 2026

final result: passed

Keep is coral red on the left. Sell is green on the right, replacing Rehome, and sells immediately without a confirmation page. Growing fish and eggs show timer badges; hungry fish show Growth paused in amber. Locked actions use muted colors and a padlock, with the reason shown above the badge.

The approved compact mockup, previous native adult view and [updated adult view](evidence/fish-actions-badges/final-direct-sell/cowfish-1642x958.png) were reviewed together at 1642 by 958 pixels, 1x native density. Phone, portrait and tablet captures cover growth, hatching, locks, favorites, immediate selling and kept fish. Both native builds and all 15 test suites pass. No actionable P0/P1/P2 finding remains. See the [full review](evidence/fish-actions-badges/design-qa.md). Earlier reviews follow.

# Compact fish popover, 17 September 2026

final result: passed

Implemented the [approved compact layout](design/mockups/fish-popover-2026-09-17/02-palette-and-rewards.png) with growth stages, a shared coin/XP payout, Keep, Favorite and confirmed Rehome. The frame follows the selected fish and stays within safe edges. Existing fonts, theme surfaces, button colors, coin/net artwork and the shared XP badge are reused. No new runtime image, icon or font assets remain.

The [native adult capture](evidence/fish-popover-compact/final/cowfish-1642x958.png) and source were reviewed together at 1642 × 958 pixels, 1x density, with matching adult Cowfish rewards. [Full](evidence/fish-popover-compact/comparison-full-final.png) and [focused](evidence/fish-popover-compact/comparison-details-final.png) comparisons cover typography, layout, colors, image quality and copy. The native frame is 469.7 × 314.5 pixels. Phone, tablet, portrait and action states were also checked. Earlier crowded labels, uneven marker spacing and a missing heart glyph are fixed.

Desktop and iOS simulator builds and all 15 desktop test suites pass. The [full QA report](evidence/fish-popover-compact/design-qa.md) records comparison history, accepted asset/font differences and validation limits. Earlier reviews follow.

# Tank shop matched to Fish, 17 September 2026

final result: passed

The user's Fish shop screenshots now govern the tank layout. Tanks keeps its wider cards and shares Fish's filter styling, enclosing frame, card surfaces, text sizes, spacing, price controls and bottom scrollbar. A selected All filter is included. Cards clip inside the frame at both scroll limits, and portrait card text remains readable.

The [Fish reference](evidence/tank-shop-consistency/final/fish-reference.png) and [updated Tanks view](evidence/tank-shop-consistency/final/tanks-matched.png) were reviewed together at 1088 by 635 pixels, 1x native SDL density, in the same level-1 state with 45 coins and 0 pearls. CSS dimensions do not apply. Typography, spacing, colors, image quality and copy were checked, along with phone, tablet, portrait and scrolled views. Earlier frame, filter, styling, clipping and portrait type issues are fixed. No actionable P0/P1/P2 finding remains.

Desktop and iOS simulator builds and all 15 desktop test suites pass. The [full review](evidence/tank-shop-consistency/design-qa.md) records evidence, fixes and validation limits. Earlier reviews follow.

# Tank shop cards and consistent height, 17 September 2026

final result: passed

Implemented the [approved tank card mockup](design/mockups/tank-shop-cards-2026-09-17/tank-shop-cards-v3.png), with slots above the artwork and side-by-side purchase choices. The later height request is included: Tanks shares the same card top edge and height as Fish, Plants, Decorations and Treasure. The scrollbar sits above the cards, and the Tanks tab remains after Decorations.

The source and [final native rendering](evidence/tank-shop-cards/consistent-height/cards-approved.png) were opened together at 1643 by 957 pixels, 1x density, in the same level-7 state. Native SDL coordinates apply; CSS dimensions do not. Full-size text and artwork were readable without detail crops. The [Decorations comparison](evidence/tank-shop-cards/consistent-height/height-decorations.png) confirms the shared vertical bounds. Phone, tablet, portrait, compact and scrolled states were also reviewed.

Typography, spacing, colors, image quality and copy pass. Earlier P2 findings in heading sizes, card treatment, button art and phone spacing were fixed and recaptured. There are no remaining actionable P0/P1/P2 findings. Minor font and illustration differences remain acceptable P3 polish. Both desktop and iOS simulator builds pass, as do all 15 desktop test suites. The native window automation tool timed out, so no manual window-interaction pass is claimed. Production SDL screenshots and event tests provide the validation evidence.

The [full QA report](evidence/tank-shop-cards/design-qa.md) records the comparison history, screenshots, interactions and limits. Earlier reviews follow.

# Octopus coin and pearl dialogs, 17 September 2026

final result: passed

Both funds dialogs now use the selected orange octopus. The live amount is above the artwork, the header is blank, and the shop hint is removed. The existing outline and controls remain. The pearl version holds an iridescent pearl and uses a purple amount.

Source visual truth: [approved mockup](design/mockups/funds-octopus-2026-09-17/approved-coins.png), 1391 × 1131 pixels. Actual game screenshots: [coins](evidence/funds-octopus/coins-1088x635.png) and [pearls](evidence/funds-octopus/pearls-1088x635.png), each 1088 × 635 pixels. The idle states show 101 coins and 12 pearls.

[Coin](evidence/funds-octopus/comparison-coins.png) and [pearl](evidence/funds-octopus/comparison-pearls.png) comparison boards put the source and production native rendering together at equal 840-pixel frame widths. The boards are 1840 × 830 pixels at 1× native SDL density. The production layout uses a 1608 × 908 logical viewport. CSS dimensions do not apply. Text and control details are readable at this scale, so separate detail crops were not needed.

The five required surfaces passed review: typography, spacing, colors, image quality and copy. The first 320 × 240 review found a P2 overlap between tentacles and Open Shop. Scene scaling and bounds constraints fixed it; the [final compact capture](evidence/funds-octopus/coins-320x240.png) and layout tests confirm the fix. No actionable P0/P1/P2 finding remains. Minor font, antialiasing and illustration color differences from the generated mockup are recorded as P3 polish. The original native frame and controls are intentionally preserved.

The desktop build and the funds interaction/layout and rendering suites pass. Both artwork versions were checked at six viewport sizes. The [full QA report](evidence/funds-octopus/design-qa.md) records the evidence, comparison history, controls tested and limits. Earlier reviews follow.

# All six tanks in the switcher, 17 September 2026

final result: passed

The expanded switcher now always shows all six tanks. The active tank has a Current badge, owned tanks switch directly, and locked tanks open their highlighted Shop card. Tank 6 remains Coming soon. Eight relevant test suites pass. [Native captures and validation](evidence/tank-switcher-all/design-qa.md) cover the updated states. Earlier reviews follow.

# Larger porthole tank switcher, 17 September 2026

final result: passed

Implemented the selected larger porthole switcher in the Clay HUD. The original Tank button stays in place, owned tanks switch directly, and buying and upgrading move to Shop's Tanks tab. Tank 6 shows Coming soon without a price or purchase action. The native build and seven relevant test suites pass. [Source comparison, state captures and validation](evidence/tank-switcher/design-qa.md) document the visual review and minor remaining illustration differences. Previous reviews follow.

# Smaller sell reward ribbon, 17 September 2026

final result: passed

The coin and XP ribbon beneath each fish now uses 50% of its previous width and height, including live text, icon artwork and spacing. Reviewed `evidence/half-size-rewards/1672x941/ribbon-tank.png` and `evidence/half-size-rewards/667x375/stage-1.png`. Existing care and selling checks pass at four viewport sizes. Previous reviews follow.

# Small fish stage meter, 17 September 2026

final result: passed

Implemented the accepted compact meter with a stage number, progress to the next stage and a lock for fish that cannot sell. The large Growing badge and footer helper are absent. See [native stage meter QA](evidence/stage-meter/design-qa.md). Previous reviews follow.

# Exact sell ribbon review, 17 September 2026

final result: passed

Shape scaling and text alignment are corrected. The user accepted the current appearance, so no further source-art extraction is needed. See [revision review](evidence/sell-ribbon-revision/design-qa.md). Prior reviews follow.

# Sell reward ribbon update, 17 September 2026

final result: passed

The selected cream and teal ribbon is implemented with live reward text and the shared Done button. See [native ribbon QA](evidence/sell-ribbon/design-qa.md). Previous reviews are preserved below.

# Clay tank grid update, 16 September 2026

final result: passed

The selected six-card design is implemented in the current Clay HUD, with the user’s later shop-style and locked-state changes. Review: [tank grid QA](evidence/tank-grid/design-qa.md). Previous reviews are preserved below.

# Fish popover reference update

final result: passed

The 14 September 2026 fish popover update is documented in [the popover design review](evidence/fish-popover-match/design-qa.md). The live popover follows the supplied frame, typography, growth card, reward rows and Rehome button. Source-aligned Metal captures and seven relevant UI suites were reviewed. The report records the dynamic balance values and minor rendering differences. Earlier reviews are preserved below.

# General message dialog update

final result: passed

The 14 September 2026 Coin and Pearl dialog update is documented in [the general dialog review](evidence/general-dialog/design-qa.md). Both shortages use a shared frame measured from the supplied reference, with live amounts, matching illustrations, Open Shop, and a red X. Desktop and iOS simulator builds passed. Seven regression suites and the final native currency checks passed. Earlier reviews are preserved below.

# Shop dialog size update

final result: passed

The 14 September 2026 Shop size update is documented in [the Shop size review](evidence/shop-dialog-size/design-qa.md). The Shop now shares the tank dialog's frame sizing, with expanded vertical spacing and reused artwork. Native desktop, phone, and tablet previews were reviewed, and six relevant test suites passed. Earlier reviews are preserved below.

# Tank menu reference update

final result: passed

The 14 September 2026 tank menu update is documented in [the tank menu design review](evidence/tank-menu-reference/design-qa.md), with native comparisons and interaction checks. The menu follows the supplied two-column reference and reuses the shared panel, fonts, animation, and purchase logic. Earlier reviews are preserved below.

# Current redesign

The 13 September 2026 Lagoon restyle is documented in [the latest design review](evidence/lagoon-redesign/design-qa.md), with native screenshots and test results. Shared assets and reuse rules are in [assets/lagoon/README.md](assets/lagoon/README.md). The prior review below is preserved for context.

# Currency shop text and spacing QA

final result: passed

Reviewed on 12 September 2026. This pass addresses the user's follow-up: remove blue artifacts in item text, remove the top tabs, and adjust the space. The earlier reference review is preserved in `evidence/currency-shop-cleanup/prior-design-qa.md`.

## Reference and actual state

- Original reference: `work/currency-shop-reference/reference.png`, 1752 × 980 pixels. The user also supplied a crop of the three coin headings at 8:10 PM to identify the issue.
- Final native capture: `evidence/currency-shop-cleanup/shop-native.png`, 1674 × 930 pixels. The app ran in a window of 837 × 465 points at 2× density, using the Metal renderer.
- Comparison: `evidence/currency-shop-cleanup/heading-comparison.png` shows the source headings above the new headings. Both strips use the same source-pixel scale. The full native capture and this comparison were opened and reviewed.
- State: settled Currency Shop, all seven preview offers visible, no purchase dialog. There are no category tabs. The fixture has 14 XP; the supplied reference has 0 XP.

## Findings and fixes

| Finding | Fix | Verification |
| --- | --- | --- |
| Blue outlined item lettering was embedded in the card images. | Remove headings, descriptions, amounts and prices from all seven sprites. Render white native copy with a subtle shadow. | Source/actual heading comparison and final full capture. |
| Original lettering could remain beneath a pressed card. | Use an empty frame and text-free card sprites in every frame. Apply the same press transform to the sprite and its native copy. | Native pressed and release captures for coin and bundle cards. All seven offers checked through input tests. |
| Removing tabs left unnecessary height and could crowd the close control. | Move both card rows and the footer up 44 source pixels. Reduce the panel height from 854 to 810 pixels, retaining clearance below Close. | Native captures, card bounds and safe-window checks at seven sizes. |
| Source water around the footer formed a rectangular edge. | Give the Earn Coins sprite a rounded alpha mask. | Final native capture and phone-size capture. |

No actionable P0, P1 or P2 issue remains for this correction. The item illustrations, card proportions, amounts, prices and ribbon wording follow the source. Native lettering and the shorter panel are intentional changes requested by the user. This pass does not claim an exact pixel match for the changed typography or the live aquarium around the menu.

## Checks

- Desktop build passed. Log: `evidence/currency-shop-cleanup/build.log`.
- Native currency suite passed at 667 × 375, 852 × 393, 1024 × 768, 1472 × 744, 1472 × 950, 600 × 800 and 1674 × 930 window sizes. Log: `native-tests.log` in the same evidence folder.
- Purchase and save controls, menu motion, and startup/shop swipe regression suites passed on the native renderer. Log: `regressions-native.log`.
- Checks cover absent tab targets, native offer labels, compact panel proportions, nonoverlapping cards, safe bounds, minimum offer touch size, all seven matching previews, mouse/touch actions, release outside, Earn Coins, close/return, Escape and an unchanged wallet.
- A headless software pass also passed before the final spacing refinement. Native checks cover the final spacing.
- Nine runtime artwork entries passed SHA-256, dimension and alpha checks. Record: `asset-integrity.json`.
- Native idle, pressed and release captures were reviewed, including 667 × 375 landscape and 600 × 800 portrait windows. The seven-card landscape composition stays centered and scales to fit in portrait.
- The heading comparison uses actual native output. No screenshot was substituted for the running interface.

## Scope

ImageGen supplied the blank lettering material and empty panel face. Generated files and prompts are retained under `work/currency-shop-reference/`. The preparation script keeps original illustration pixels outside the text patches. The title, red promotion ribbons and Earn Coins copy retain their original artwork.

The app uses its existing aquarium, HUD and navigation. Payments and rewarded ads remain unconnected. No physical phone, simulator or screen-reader check was performed in this pass. The native preview uses the shop fixture without changing the ordinary save.
