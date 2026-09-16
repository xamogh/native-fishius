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
