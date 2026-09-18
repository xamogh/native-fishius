# Current game interface

The default game interface uses Clay's row and column layout. The old interface and Studio editor have been removed. Clay calculates the positions and sizes. SDL draws the panels, text and future artwork into those boxes.

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
- `src/main.cpp` opens this interface by default. `--hud-layout` remains an optional compatibility alias.

Player level, XP and wallet balances come from the game state. The Tank button uses the same size and styling as Shop, with no arrows or capacity counter. Placeholder icon slots can later display new artwork without changing the layout tree. The canonical fish, plant and decoration files remain intact.

## Preview

From the project root:

```sh
./Run\ Aquarium.command --fixture aquarium
```

The fixture runs in a separate review session and does not overwrite the player's save. Shop, tank controls, feeding and fish care actions use the live game state. Bag, Projects, Rewards and Settings still open placeholder dialogs.

## Checks

`aquarium_hud_tests` checks phone, tablet, desktop and portrait sizes, including a notched phone safe area. It checks control overlap, safe-area boundaries, scaled control sizes, hit targets and flexible aquarium space.

Render captures and reports are in `evidence/clay-hud/`. The 667 by 375, 1024 by 768 and 390 by 844 previews were visually inspected.

The right action column stacks Rehome, Food, Projects, Bag and Shop from top to bottom, aligned on the right. Bag is 75% of Shop in both dimensions. The Rehome net sits immediately above Food and uses the same 64-unit control size and 16-unit gap as the rest of the column.

## Feeding and rehoming

Choose Food, then tap the water to drop one free pellet. Hungry fish chase and eat pellets through the normal movement simulation. Eating resumes growth and saves care progress. Tap Food again, choose Done or press Escape to stop. Opening a menu also turns the tool off. Press F to toggle feeding or S to toggle the Rehome net. Either tool ends egg placement without buying an egg.

With no tool active, tap a fish or egg to open its details. The dialog shows care, growth, the current sale reward and the reward at adulthood. Adult rewards show the coin and XP totals without a purchase-cost or profit breakdown. Keep sits on the left in coral red, and Sell sits on the right in green. Locked actions use a muted surface and a padlock. Growth time appears in a mint badge with a clock; hungry fish show an amber Growth paused badge. Eggs show their hatching time in the same badge style. Eggs and Babies show Unlocks at Junior in a separate lavender badge above the timer.

Sell pays the current coin and XP reward and closes the popover in one tap, without a confirmation page. The reward follows live growth while the details are open. Selling unlocks at Junior (age 1). Eggs and Babies cannot be sold. Favorites must be unfavorited before selling. The details explain why Sell is locked.

The growth percentage updates live to two decimal places, such as 17.42%. It uses the fish's actual growth time and stays fixed while growth is paused. It reaches 100.00% only when growth is complete.

While the Rehome net is active, eligible fish show the compact cream and teal reward ribbon below them. A small meter above each fish follows its width, with a gold square showing its current stage (0 Baby through 4 Adult) and a thin aqua bar showing progress to the next stage. The bar resets at each stage and stays full for adults. Eggs show an egg in the square and progress to hatching. A small shared lock icon marks eggs, Babies, favorites and kept fish; these have no reward ribbon or large status badge. There is no helper text above Done. Tap an eligible fish once to rehome it immediately. The net stays active for repeated sales until Done, Escape, a second net tap or another menu. Dragged and cancelled gestures do not sell fish.

Adult growing fish offer Keep, which pays the same one-time reward and retains a display fish. A full display area sends a kept fish to Bag under the existing rules. Display fish and premium companions cannot be rehomed for another reward. All reward actions use the saved settlement ledger. A failed save leaves the fish and balance unchanged and keeps the details open for retry.

Successful Sell, Rehome net and Keep actions send gold coins and blue XP stars from the fish into the top HUD. The icons rise, spread, then curve toward the coin icon and XP bar in a staggered stream. They shrink into the targets, which briefly glow and bounce. The displayed coin total counts up and the XP bar fills as each icon arrives. Multiple sales can animate together. Zero XP creates no XP stars, including at the maximum level. Level notices follow the displayed XP.

Rewards are saved before the animation starts. `hud_rewards.cpp` delays only their presentation, so closing the game cannot lose a reward. Replayed requests and failed saves produce no extra effects. Opening Shop finishes active flights and shows the saved balance immediately. Reduced motion replaces flight and bounce with a brief highlight and immediate totals. Particle paths use the current HUD bounds after window or safe-area changes. `aquarium_hud_rewards_tests` checks arrival timing, repeated sales, spending during flight, level changes, maximum XP, reduced motion and menu changes. Its optional capture folder contains desktop, phone, tablet, portrait and notched-screen frames.

`hud_care.cpp` owns care gestures, fish details, tool feedback and sale actions. Mouse and touch use the same control bounds. Only the first finger owns a gesture; SDL's duplicate mouse events are ignored. Drags, cancelled touches, focus loss and taps on menus cannot feed or rehome a fish. Tool cursors reuse the existing artwork under `assets/lagoon` and `assets/tools`. Feeding shows the active Food highlight and Done button without an instruction banner.

Rehome uses the generated `assets/hud-icons/rehome-button-v1.png` tile. Its cream and gold frame and painted teal net follow the Food, Bag and Shop artwork. The renderer fits its visible bounds to the same 64-unit size as Food. The PNG retains its original generated alpha without background removal or image edits. The prompt, source and alpha measurements are recorded in `assets/hud-icons/generation-rehome-v1.json`.

`aquarium_hud_care_tests` checks feeding, consumption, growth, rewards, immediate selling, favorites, Keep, the Junior sale threshold, full tanks, touch ownership and persistence. It also checks that saving failures preserve the fish. Optional captures are written to the directory supplied as its second argument. Software captures at 667 by 375, 1024 by 768 and 390 by 844 are stored in `evidence/clay-care/`.

Projects uses the same 108 by 108 design-unit tile as Bag and sits immediately above it. It is a placeholder button in the HUD interface.

## Shop page

Clicking Shop opens a full-screen Clay page with a 64 by 64 close button at the top right. Click X or press Escape to return. The shop has Fish, Plants, Decorations and Treasure tabs, a heading, two sub-tabs, a horizontal product row and live coin and pearl balances. Drag the product row or use the mouse wheel to scroll. Category and sub-tab changes reset scroll. All and Unlocked filter the catalog by player level; Treasure has Coins and Pearls tabs. Fish cards start placement and charge their live price only when placed. Other catalog prices remain informational. Treasure has five offers per currency, with exact amounts and proposed USD prices imported from v4. Tapping a pack opens a preview without changing the wallet. Purchases remain unavailable. See `treasure-shop.md` for the offer table and coin calculation. Use `--fixture shop` to capture the open page.

Open a specific shop preview with `--fixture shop --ui-variant fish|plants|decorations|coins|pearls`.

Fish cards show the first growth milestone, total time to adulthood and adult coin and XP rewards. These values and coin prices come from the live purchase quote at the player's level. Only locked fish show an unlock level. Companions show that they are already adult and have no sale rewards. Durations keep hours and minutes instead of rounding long cycles into days.

Fish cards use a cream surface, warm brown labels and green price controls. A drawn clock marks the first growth time beside the adult time. `shopTheme::badge` is a reusable text badge with a selectable surface style. The orange Fast Growing badge appears when the first growth milestone is at or below five minutes, including exactly five minutes. It does not apply to companions. The card keeps a consistent badge area so prices stay aligned across the row.

Tap the top-left information button on a fish card to open its details, including price, feeding interval and rewards by growth stage. Tapping elsewhere on the card selects that fish for placement. Card drags scroll without selecting an item. Times start at purchase and assume regular feeding. Preview details with `--fixture shop --ui-variant fish-details`.

`hud_placement.cpp` handles egg placement using the existing purchase command and quote. Selecting a card does not spend currency. The main HUD stays visible. Each aquarium tap buys and places one egg while keeping the same fish selected. Done or Escape ends placement and leaves the aquarium open. Each successful drop shows floating currency cost and actual XP earned; current purchases award zero XP. Availability, level, funds, capacity and quote checks are enforced by the domain. Permanent companions retain their adult-fish placement behavior. Preview a selected egg with `--fixture shop --ui-variant fish-placement`.

### Sea-glass shop theme

`src/shop_theme.cpp` draws the deep teal header, ivory content area, mint footer, aqua cards and beveled buttons. `Canvas::gradient` and `Canvas::wave` provide scalable color fills and soft water highlights. Bubbles are also drawn in code. The layout, scrolling and shared currency HUD keep their existing dimensions and behavior. Tank uses the same themed footer and close button.

The four `assets/hud-icons/shop-tab-*-v1.png` illustrations are transparent RGBA images. Their visible alpha bounds control fitting, so the art has consistent padding without cropping or changing the original files. Labels stay in code. Generation prompts and source paths are recorded in `assets/hud-icons/generation-shop-tabs-v1.json`.

## Tank switcher and Shop tab

The original 144-unit Tank button opens a floating porthole switcher above it. All six tanks keep fixed positions in three columns and two rows, with cream frames and live tank numbers. The My Tanks header shows the number owned out of six. The active tank has a gold rim and a Current badge. Other owned tanks switch directly. Unowned tanks have dimmed artwork and a lock; tapping one opens Shop's Tanks tab with the matching card outlined in gold. Tank 6 also says Coming soon. Opening its card does not offer a purchase.

Tap Tank again to collapse, tap outside, or press Escape. Number keys 1 to 6 perform the same action as tapping the matching tank. Dragging or losing focus cancels a pending selection. Other HUD buttons remain one click away. Opening a locked tank's Shop card does not switch tanks, spend currency or change the save.

The shop tabs are Fish, Plants, Decorations, Tanks, Treasure and Backgrounds. Tanks keeps its wider cards and uses the same shop frame, card surfaces, text sizes, insets, gaps, price styling and bottom scrollbar as Fish. A single selected All tab sits in the shared filter row; tapping it resets tank browsing. Card height and top edge match the other catalog tabs. A shared clipping area keeps scrolling cards inside the surrounding frame.

Each tank card shows its name and status, growing slots at the top, an aquarium preview, and side-by-side coin and pearl choices below. The current tank has a thin gold border. Landscape shows three complete cards and part of the next; portrait shows one large card and part of the next. Drag or swipe the row, use the mouse wheel, drag the scrollbar, or use Left, Right, Home and End to browse. Off-screen controls cannot be clicked, and dragging a price never purchases it. Opening a locked tank from the switcher scrolls its complete card into view. Purchase controls use the same visible height and bottom inset as Fish prices, with expanded touch areas that remain at least 44 points at the checked screen sizes. The scrollbar does not overlap those targets.

Tank 1 remains the free, upgradeable starter. Existing level, prerequisite, capacity and currency rules still apply through Session. Tank 6 shows Coming soon and has no purchase action until the catalog defines its prices and unlock level. The Tank full message opens this tab directly. A currency shortage keeps the tank scroll position when visiting Treasure and returning. The original card mockup is recorded in [the card layout review](../evidence/tank-shop-cards/design-qa.md); the later Fish shop alignment is recorded in [the consistency review](../evidence/tank-shop-consistency/design-qa.md).

The generated sprites, blank count header, reusable cream and gold rims and geometry manifest are in `assets/tank-switcher`. Live labels remain native text. The selected reference and native comparisons are recorded in [the porthole review](../evidence/tank-switcher/design-qa.md). The later all-tank navigation and badge changes are recorded in [the all-tanks review](../evidence/tank-switcher-all/design-qa.md).

Preview the switcher with `--fixture tank-switcher`. Preview Shop's Tanks tab with `--fixture tanks` or `--fixture shop --ui-variant tanks`. These fixtures do not overwrite a player's save. The catalog defines five tanks. Tank 6 is visible as a Coming soon preview and cannot be bought. The test capture with six owned tanks remains an ephemeral comparison with the original mockup.

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

The full-screen Shop and floating Tank switcher use their own layouts alongside these reusable modal dialogs.

Coin and pearl shortages use the shared prompt in `hud_funds.cpp`. Fish selection, repeated egg placement, tank purchases and upgrades, and background purchases show the exact missing amount above an octopus illustration and an **Open Shop** button below. The number is gold for coins and purple for pearls. The octopus holds the matching currency. The blue header says **Not enough coins** or **Not enough pearls**, or **Not enough coins and pearls** when both are needed. There is no shop hint sentence. The button opens Treasure on the matching Coins or Pearls tab. Closing Treasure returns to the previous catalog, filter and scroll position, or to the aquarium when the purchase started there. Failed purchases do not charge the wallet. A shortage during egg placement ends placement before opening the prompt.

The prompt uses the current dialog frame and generated underwater artwork. Text and buttons remain native controls. The scene scales down when larger touch targets need more room. X, Escape and the backdrop dismiss it. Enter or Space opens the shop. Mouse and touch share the same targets; a drag or interrupted press cannot open the shop. The X and shop button retain a full touch target on phone screens. Preview the prompts with `--fixture shop --ui-variant not-enough-coins` or `--fixture shop --ui-variant not-enough-pearls`. `aquarium_hud_funds_tests` checks purchase routes, exact amounts, navigation, cancellation and responsive layout.

Bag and Projects open the large 1280 by 768 dialog with their own titles and blank content areas. Both close via X or Escape.

## Generated HUD artwork

Shop uses the approved thin-border `assets/hud-icons/shop-button-v3.png` with true generated alpha and a static rim highlight. Tank uses matching `tank-button-v3.png`, fitted by its visible alpha bounds to the original 144-unit button. Rewards uses matching icon-only `rewards-button-v3.png`, fitted to its original 64-unit bounds. Food uses matching icon-only `food-button-v3.png`, fitted to its original 64-unit bounds. Bag and Projects use matching icon-only v3 buttons fitted to their original 108-unit bounds. Settings retains its existing artwork. The border highlight is not animated.

Coin and Pearl use transparent v4 cartoon currency sprites, fitted to their existing HUD and menu wallet bounds. Bar sizes and overlaps remain unchanged.

Shop footer wallets share the main HUD layout and renderer, translated to the footer with plus buttons hidden. Rewards and Settings open large blank reusable dialogs with their own titles, closed with X or Escape.

The sell reward ribbon uses `assets/hud-icons/sell-reward-ribbon-v1.png`, fitted to its recorded visible bounds. Its coin medallion and parchment/teal artwork are generated; the coin amount and XP are live text. At the user's request, the ribbon now renders at half the original width and height, including its text, icons, outlines and spacing. The minimum label width on narrow screens is about 44 pixels. Done reuses the same Buy surface and white label as feeding and egg placement.
