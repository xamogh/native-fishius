# Mobile-first level receipt

The user selected the first image in the latest three-image set on 18 September 2026. This supersedes the earlier horizontal cabinet layout.

Source: `exec-ca335ab9-ad9e-411c-9ae6-dfcf33e1faf5.png`, created with built-in Create Image. The unchanged selected result is saved as [approved.png](approved.png), 1844 × 852 pixels, designed for a landscape phone at 844 × 390 points.

The shared blue frame, navy wave header and red close button stay native. The left 30% of the body holds the level medal, earned currencies and Continue. A vertical divider separates a single horizontal gallery, with two square cards and a partial next card. Position marks follow the scroll. At narrow portrait sizes the receipt sits above the gallery, with Continue below it.

## Measured reference

Coordinates below are reference image pixels. Runtime scales them into safe phone space and retains 44-point action targets.

| Region | Reference bounds (x, y, width, height) |
| --- | --- |
| Frame | 155, 80, 1538, 700 |
| Body | 160, 194, 1525, 578 |
| Left receipt and action | 187, 209, 418, 550 |
| Vertical divider | 626, 220, 2, 536 |
| Gallery heading | 666, 219, 732, 58 |
| First square card | 660, 291, 396, 394 |
| Second card | 1078, 291, 400, 394 |
| Continue | 187, 665, 417, 94 |

## Asset inventory

| Element | Implementation |
| --- | --- |
| Quiet cream and aqua interior | New `dialogs/level-up-mobile-background-v1.png` |
| Square aquarium card surface | New `dialogs/level-up-mobile-card-v1.png` |
| LEVEL ribbon, gold medal, sea plants | New `dialogs/level-up-mobile-badge-v1.png`; live number on blank face |
| Fish, plants, decor and tank | Existing catalog artwork, with its original proportions |
| Gold coin and iridescent pearl | Existing `hud-icons/coin-v4.png` and `hud-icons/pearl-v4.png` |
| Frame, close, Continue and position marks | Existing native UI primitives and theme |
| Text | Existing Lilita One content font and Luckiest Guy shared controls |

All three new art assets were created with built-in Image Gen. Their sibling provenance JSON files record prompts and generated sources. All labels, counts, card names, scrolling and actions remain live UI. The 500 ms reward animation uses the new receipt icon positions and does not credit rewards again.

The user later simplified the reward copy: show only the coin or pearl icon and its number, with no plus sign or currency name. The shared “You received” heading remains.
