# Clay aquarium HUD

The new HUD uses Clay's row and column layout. Clay calculates the positions and sizes. SDL draws the panels, text and future artwork into those boxes.

## How it relates to flexbox

| Layout idea | Clay setting |
| --- | --- |
| Horizontal row | `CLAY_LEFT_TO_RIGHT` |
| Vertical column | `CLAY_TOP_TO_BOTTOM` |
| Use remaining space | `CLAY_SIZING_GROW()` |
| Fit the children | `CLAY_SIZING_FIT()` |
| Set a control size | `CLAY_SIZING_FIXED()` |
| Space between children | `childGap` |
| Space inside a group | `padding` |
| Align children | `childAlignment` |

This is native C++ layout. It does not require HTML or a browser.

## Layout tree

```text
Safe area: column that fills the window
  Top row
    Level and player progress
    Flexible space
    Coins, pearls and Settings
  Middle row: grows vertically
    Aquarium: grows in both directions
  Bottom row
    Tank button
    Flexible space
    Food above Shop: right-aligned column
      Food matches the Settings and XP image size
```

The same rows and columns remain at every window size. Controls and text scale down on small screens instead of stacking into new rows. Device safe areas still apply. The XP, coin and pearl images overlap their bars by 8 window points, and the pearl track is half the width of the coin track.

## Four-unit design grid

Authored HUD sizes, spacing, padding, corner radii, border widths and text sizes use multiples of four. They are defined in `include/aquarium/hud_tokens.hpp`; off-grid token values fail at compile time.

| Measurement | Design units |
| --- | --- |
| Shop | 144 by 144 |
| Food | 64 by 64 |
| Currency images and Settings | 64 by 64 |
| XP track | 344 by 40 |
| Coin track | 240 by 40 |
| Pearl track | 120 by 40 |
| Tank button | 144 by 144, matching Shop |
| Bag | 108 by 108 |
| Group gap | 16 |
| Panel padding | 12 |
| Outer margin | 24 |
| Text sizes | 20, 24, 32 |
| Corner radii | 8, 16 |
| Borders | 4 |

The currency widths round the previous roughly 30-percent reduction to nearby grid values. Food matches the 64 by 64 Settings and XP image size, and the pearl track stays exactly half the coin width.

The existing image overlap remains 8 window points. Responsive scaling, safe areas, centered placement and growing containers can produce fractional final coordinates. Progress fill remains continuous instead of jumping in four-unit increments.

## Code

- `include/aquarium/hud.hpp` defines named HUD slots and the layout result.
- `src/hud.cpp` builds the Clay tree in `layoutHud()`. It has no fixed X/Y positions for HUD controls.
- `paintHud()` draws simple panels, placeholder icon slots and live game values.
- `hudHit()` uses the same rectangles as the painter for hover and press feedback.
- `src/main.cpp` exposes the layout through `--hud-layout`.

Player level, XP and wallet balances come from the game state. The Tank button uses the same size and styling as Shop, with no arrows or capacity counter. Placeholder icon slots can later display new artwork without changing the layout tree. The existing fish, plant and decoration files remain intact.

## Preview

From the project root:

```sh
./Run\ Aquarium.command --hud-layout --fixture aquarium
```

The fixture runs in a separate review session and does not overwrite the player's save. The layout preview has hover and press feedback. Menu navigation and game actions are not connected in this pass.

## Checks

`aquarium_hud_tests` checks phone, tablet, desktop and portrait sizes, including a notched phone safe area. It checks control overlap, safe-area boundaries, scaled control sizes, hit targets and flexible aquarium space.

Render captures and reports are in `evidence/clay-hud/`. The 667 by 375, 1024 by 768 and 390 by 844 previews were visually inspected.

The right action column stacks Food, Projects, Bag and Shop from top to bottom, aligned on the right. Bag is 75% of Shop in both dimensions.

Projects uses the same 108 by 108 design-unit tile as Bag and sits immediately above it. It is a placeholder button in the HUD layout preview.

## Shop page

Clicking Shop opens a full-screen Clay page with a 64 by 64 close button at the top right. Click X or press Escape to return. The shop has Fish, Plants, Decorations and Treasure tabs, a heading, two sub-tabs, a horizontal product row and live coin and pearl balances. Drag the product row or use the mouse wheel to scroll. Category and sub-tab changes reset scroll. All and Unlocked filter the catalog by player level; Treasure has Coins and Pearls tabs. Fish cards start placement and charge their live price only when placed. Other catalog prices remain informational. Treasure packs use placeholder artwork and Coming soon labels. Use `--hud-layout --fixture shop` to capture the open page.

Open a specific shop preview with `--hud-layout --fixture shop --ui-variant fish|plants|decorations|coins|pearls`.

Fish cards show the first growth milestone, total time to adulthood and adult coin and XP rewards. These values and coin prices come from the live purchase quote at the player's level. Only locked fish show an unlock level. Companions show that they are already adult and have no sale rewards. Durations keep hours and minutes instead of rounding long cycles into days.

Fish cards use a cream surface, warm brown labels and green price controls. A drawn clock marks the first growth time beside the adult time. `shopTheme::badge` is a reusable text badge with a selectable surface style. The orange Fast Growing badge appears when the first growth milestone is at or below five minutes, including exactly five minutes. It does not apply to companions. The card keeps a consistent badge area so prices stay aligned across the row.

Tap the top-left information button on a fish card to open its details, including price, feeding interval and rewards by growth stage. Tapping elsewhere on the card selects that fish for placement. Card drags scroll without selecting an item. Times start at purchase and assume regular feeding. Preview details with `--hud-layout --fixture shop --ui-variant fish-details`.

`hud_placement.cpp` handles egg placement using the existing purchase command and quote. Selecting a card does not spend currency. The main HUD stays visible. Each aquarium tap buys and places one egg while keeping the same fish selected. Done or Escape ends placement and leaves the aquarium open. Each successful drop shows floating currency cost and actual XP earned; current purchases award zero XP. Availability, level, funds, capacity and quote checks are enforced by the domain. Permanent companions retain their adult-fish placement behavior. Preview a selected egg with `--hud-layout --fixture shop --ui-variant fish-placement`.

### Sea-glass shop theme

`src/shop_theme.cpp` draws the deep teal header, ivory content area, mint footer, aqua cards and beveled buttons. `Canvas::gradient` and `Canvas::wave` provide scalable color fills and soft water highlights. Bubbles are also drawn in code. The layout, scrolling and shared currency HUD keep their existing dimensions and behavior. Tank uses the same themed footer and close button.

The four `assets/hud-icons/shop-tab-*-v1.png` illustrations are transparent RGBA images. Their visible alpha bounds control fitting, so the art has consistent padding without cropping or changing the original files. Labels stay in code. Generation prompts and source paths are recorded in `assets/hud-icons/generation-shop-tabs-v1.json`.

## Tank dialog

Tank opens a full-screen panel with six blank cards in three columns and two rows. Clay distributes the available space equally, with 24-unit padding and gaps. It shares Shop’s wallet bar and 64 by 64 close button through the same renderer and layout. Click X or press Escape to return. Preview with `--hud-layout --fixture tanks`.

## Reusable dialogs

`include/aquarium/hud_dialog.hpp` defines the dialog shell. All dimensions below are design units.

| Preset | Width | Height |
| --- | --- | --- |
| Small | 640 | 392 |
| Medium | 960 | 640 |
| Large | 1280 | 768 |

Use `DialogSpec` to set a title and preset. Custom dimensions must be at least 240 by 192 and round up to the next multiple of four. Dialogs center inside the safe area and scale down to fit. The 64 by 64 close button sits inside an 80-unit header. The content area has 24-unit padding.

`layoutDialog` returns the frame, header, title, close button, body and padded content bounds. Place each dialog's content inside `content`, and clip drawing to it when needed. `paintDialog` renders the dim backdrop and frame. `dialogEvent` consumes input while open, closes on X or Escape, and prevents clicking through to the underlying screen. Clicking the backdrop closes the dialog. The press and release must both be outside the frame; clicks inside and drags from inside to outside do not dismiss it. Dismissal consumes the input so it cannot activate the screen underneath.

Preview examples:

```sh
./Run\ Aquarium.command --fixture aquarium --hud-dialog small
./Run\ Aquarium.command --fixture aquarium --hud-dialog custom --dialog-width 800 --dialog-height 480 --dialog-title "Tank details"
```

The existing full-screen Shop and Tank screens remain separate from these reusable modal dialogs.

Bag and Projects open the large 1280 by 768 dialog with their own titles and blank content areas. Both close via X or Escape.

## Generated HUD artwork

Shop uses the approved thin-border `assets/hud-icons/shop-button-v3.png` with true generated alpha and a static rim highlight. Tank uses matching `tank-button-v3.png`, fitted by its visible alpha bounds to the original 144-unit button. Rewards uses matching icon-only `rewards-button-v3.png`, fitted to its original 64-unit bounds. Food uses matching icon-only `food-button-v3.png`, fitted to its original 64-unit bounds. Bag and Projects use matching icon-only v3 buttons fitted to their original 108-unit bounds. Settings retains its existing artwork. The border highlight is not animated.

Coin and Pearl use transparent v4 cartoon currency sprites, fitted to their existing HUD and menu wallet bounds. Bar sizes and overlaps remain unchanged.

Shop and Tank footer wallets share the main HUD layout and renderer, translated to the footer with plus buttons hidden. Rewards and Settings open large blank reusable dialogs with their own titles, closed with X or Escape.
