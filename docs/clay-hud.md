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

The fixture runs in a separate review session and does not overwrite the player's save. Shop, tank controls, feeding and fish care actions use the live game state. Bag opens stored plants and decorations. Settings opens the saved preferences menu. Projects opens a placeholder dialog. Rewards shows progress toward the next pearl earned from twenty adult coin-fish sales.

## Checks

`aquarium_hud_tests` checks phone, tablet, desktop and portrait sizes, including a notched phone safe area. It checks control overlap, safe-area boundaries, scaled control sizes, hit targets and flexible aquarium space.

Render captures and reports are in `evidence/clay-hud/`. The 667 by 375, 1024 by 768 and 390 by 844 previews were visually inspected.

The right action column stacks Layout, Rehome, Food, Projects, Bag and Shop from top to bottom, aligned on the right. Bag is 75% of Shop in both dimensions. Layout and the Rehome net use the same 64-unit control size as Food, with a 16-unit gap.

## Feeding and rehoming

Drag the Food button into the tank to pour a stream of free food. Keep dragging to scatter pellets along the path, then release to stop. You can also choose Food or press F, then drag within the water. The food can follows the pointer or finger. Fast swipes leave a trail, and slow drags or pauses keep pouring about 12 pellets per second. Taps and small finger movements do not start a stream. Pouring pauses over controls or outside the water and resumes when the drag returns to the tank. The tank holds at most 48 uneaten pellets. Hungry fish chase and eat pellets through the normal movement simulation. Eating resumes growth and saves care progress. Tap Food again, choose Done or press Escape to stop. Opening a menu also turns the tool off. Press F to toggle feeding or S to toggle the Rehome net. Either tool ends egg placement without buying an egg.

With no tool active, tap a fish or egg to open its details. The dialog shows care, growth, the current sale reward and the reward at adulthood. Adult rewards show the coin and XP totals without a purchase-cost or profit breakdown. One green Sell button spans the bottom; the header heart toggles favorite protection. Locked actions use a muted surface and a padlock. Growth time appears in a mint badge with a clock; hungry fish show an amber Growth paused badge. Eggs show their hatching time in the same badge style. Eggs and Babies show Unlocks at Junior in a separate lavender badge above the timer.

Sell pays the current coin and XP reward and closes the popover in one tap, without a confirmation page. The reward follows live growth while the details are open. Selling unlocks at Junior (age 1). Eggs and Babies cannot be sold. Favorites must be unfavorited before selling. The details explain why Sell is locked.

Trying to sell a favourite shows the approved bottle-and-parchment toast at the top right, just below Settings. Both the sell net and the fish details use the same message: **This fish is a favourite**, followed by **Unfavourite it before selling.** The fish, favourite and wallet stay unchanged. X or Escape dismisses the toast. It expires after 3.5 seconds even when hovered and restarts its timer after another blocked sale. Dragged and cancelled taps do not dismiss it or act on the aquarium underneath.

The toast keeps its illustrated bottle, heart lock, parchment and close symbol in a transparent sprite. The message is live text. Its compact width stays between 224 and 288 window points where space allows, with a 44-point close target and safe-area margins. It drops from the top right, lands with a small bounce, then rises and fades out. Hit targets follow the moving artwork. Reduced motion removes the movement and fade. Opening another menu, changing tools or changing the fish's favourite clears the notice.

While the Sell tool is active, favourited fish show a small red heart above their heads. The heart follows swimming and turns and stays above the sell meter. It hides when Sell ends, another tool is selected, or the fish is unfavourited. All favourites show the same marker in Sell mode. Normal play, fish details, stored fish and other tanks do not show it.

The growth percentage updates live to two decimal places, such as 17.42%. It uses the fish's actual growth time and stays fixed while growth is paused. It reaches 100.00% only when growth is complete.

Selecting Sell or pressing S gently nudges nearby fish apart while preserving their places in the tank. Fish that already have room stay in place. After clearing the edges and controls, each fish can move up to 52.8 screen points to ease crowding, matching the 20 percent increase in fish size. Dense groups may retain some overlap instead of spreading into rows or across the tank. Fish ease into and out of the adjustment, then stay still. The spacing accounts for fish artwork, tap targets, stage meters and reward ribbons, including Babies, favourites and premium fish. Eggs keep their normal sinking behavior. A pressed fish stays fixed until the gesture ends. Selling one fish leaves the others in place. Done, Escape, another tool or a menu restores normal swimming. Resizing the window or hatching a fish updates the spacing. Destinations and transition speed are temporary and are not written into saves.

Sell mode uses light blur and dimming for the scenery, plants, decorations and food pellets. Fish, their labels and the HUD stay sharp. Leaving Sell restores the normal tank view.

While the Rehome net is active, eligible fish show the compact cream and teal reward ribbon below them. A small meter above each fish follows its width, with a gold square showing its current stage (0 Baby through 4 Adult) and a thin aqua bar showing progress to the next stage. The bar resets at each stage and stays full for adults. Eggs show an egg in the square and progress to hatching. A small shared lock icon marks eggs, Babies and favorites; these have no reward ribbon or large status badge. There is no helper text above Done. Tap an eligible fish once to rehome it immediately. The net stays active for repeated sales until Done, Escape, a second net tap or another menu. Dragged and cancelled gestures do not sell fish.

Adults remain in their tank until sold. The heart protects a favorite from selling without granting rewards or changing its type. The popover has one wide Sell button and no Keep action. All fish use the same care controls and tank capacity. Sales use the saved settlement ledger. A failed save leaves the fish and balance unchanged and keeps the details open for retry.

Successful sales from the popover or Sell net send gold coins and blue XP stars from the fish into the top HUD. The icons rise, spread, then curve toward the coin icon and XP bar in a staggered stream. They shrink into the targets, which briefly glow and bounce. The displayed coin total counts up and the XP bar fills as each icon arrives. Multiple sales can animate together. Zero XP creates no XP stars, including at the maximum level. Level notices wait for the full sale animation to finish, so the dialog cannot cover rewards still traveling to the HUD. Reduced motion shows the level notice immediately.

Rewards are saved before the animation starts. `hud_rewards.cpp` delays only their presentation, so closing the game cannot lose a reward. Replayed requests and failed saves produce no extra effects. Opening Shop finishes active flights and shows the saved balance immediately. Reduced motion replaces flight and bounce with a brief highlight and immediate totals. Particle paths use the current HUD bounds after window or safe-area changes. `aquarium_hud_rewards_tests` checks arrival timing, repeated sales, spending during flight, level changes, maximum XP, reduced motion and menu changes. Its optional capture folder contains desktop, phone, tablet, portrait and notched-screen frames.

`hud_care.cpp` owns care gestures, fish details, tool feedback and sale actions. Mouse and touch use the same control bounds. Only the first finger owns a gesture; SDL's duplicate mouse events are ignored. Food starts pouring during a drag from Food or within the water while Food is active. Pellets are spaced along fast movements, with a timed stream for slower movements. Release, cancelled touches, focus loss, window resizing, Escape and opening menus stop the stream. Leaving the water pauses pouring without building up a backlog. Drags cannot rehome a fish. Tool cursors reuse the existing artwork under `assets/lagoon` and `assets/tools`. Feeding shows the active Food highlight and Done button without an instruction banner.

Rehome uses the generated `assets/hud-icons/rehome-button-v1.png` tile. Its cream and gold frame and painted teal net follow the Food, Bag and Shop artwork. The renderer fits its visible bounds to the same 64-unit size as Food. The PNG retains its original generated alpha without background removal or image edits. The prompt, source and alpha measurements are recorded in `assets/hud-icons/generation-rehome-v1.json`.

`aquarium_hud_sell_layout_tests` checks small local adjustments, unchanged isolated fish, gentle acceleration, crowded groups without a full rearrangement, desktop and phone safe areas, held touches, repeated selling, normal swimming after exit, reduced motion and save compatibility. CTest runs it as `clay_sell_spacing`.

`aquarium_hud_care_tests` checks feeding, consumption, growth, rewards, immediate selling, favorites, adult retention, the Junior sale threshold, full tanks, touch ownership and persistence. It also checks that saving failures preserve the fish. Optional captures are written to the directory supplied as its second argument. Software captures at 667 by 375, 1024 by 768 and 390 by 844 are stored in `evidence/clay-care/`.

Projects uses the same 108 by 108 design-unit tile as Bag and sits immediately above it. It is a placeholder button in the HUD interface.

## Shop page

Fish, plants and decorations are ordered by unlock level, from lowest to highest, in both All and Unlocked. Coin and pearl items appear together at each level. Items with the same unlock level keep their catalog order.

Clicking Shop opens a full-screen Clay page with a 64 by 64 close button at the top right. Click X or press Escape to return. The shop has Fish, Plants, Decorations and Treasure tabs, a heading, filter tabs, a horizontal product row and live coin and pearl balances. Drag the product row or use the mouse wheel to scroll. Category and sub-tab changes reset scroll. All and Unlocked filter the catalog by player level; Treasure opens on All and also has Coins and Pearls tabs. All three Treasure tabs sort by USD price from lowest to highest. Fish cards start placement and charge their live price only when placed. Plant and decoration cards open a preview and charge only when Place is confirmed. Background cards buy or equip a tank background. Treasure has five offers per currency, with exact amounts and proposed USD prices imported from v4. Tapping a pack opens a preview without changing the wallet. Purchases remain unavailable. See `treasure-shop.md` for the offer table and coin calculation. Use `--fixture shop` to capture the open page.

Open a specific shop preview with `--fixture shop --ui-variant fish|plants|decorations|all|coins|pearls`.

Shop, Inventory, Tanks and the level-up gallery share swipe momentum. Cards follow the pointer during a drag, then glide and gradually slow after release. Holding the row still before releasing stops it. Tapping a moving row stops the glide without selecting a card. Wheel input keeps fractional trackpad movement and eases into place. Scrollbars stay directly attached to the pointer. Changing categories, closing a menu or losing focus cancels movement. Reduced motion keeps direct scrolling and disables the glide.

Fish cards show the first growth milestone, total time to adulthood and adult coin and XP rewards. These values and coin prices come from the live purchase quote at the player's level. Pearl prices come from the catalog. Bubble Eye Goldfish costs one pearl and can be bought repeatedly. Only locked fish show an unlock level. All fish use the same care and sale controls. Durations keep hours and minutes instead of rounding long cycles into days.

Fish cards use a cream surface, warm brown labels and green price controls. A drawn clock marks the first growth time beside the adult time. `shopTheme::badge` is a reusable text badge with a selectable surface style. The orange Fast Growing badge appears when the first growth milestone is at or below five minutes, including exactly five minutes. The card keeps a consistent badge area so prices stay aligned across the row.

Tap the top-left information button on a fish card to open its details, including price, feeding interval and rewards by growth stage. Tapping elsewhere on the card selects that fish for placement. Card drags scroll without selecting an item. Times start at purchase and assume regular feeding. Preview details with `--fixture shop --ui-variant fish-details`.

`hud_placement.cpp` handles egg placement using the existing purchase command and quote. Selecting a card does not spend currency. The main HUD stays visible. Each aquarium tap buys and places one egg while keeping the same fish selected. Done or Escape ends placement and leaves the aquarium open. Each successful drop shows the floating currency cost and, when positive, the actual XP earned. Current purchases award zero XP. Availability, level, funds, capacity and quote checks are enforced by the domain. Coin and pearl fish use the same egg placement. Preview a selected egg with `--fixture shop --ui-variant fish-placement`.

### Plant and decoration placement

Plant and decoration cards retain their catalog IDs and open the placement controls in `hud_decor_placement.cpp`. Selecting a card is free. Tap or drag in the tank to move the highlighted preview, then choose the green tick to buy one copy. The price stays in Shop. Placement has no name or price banner. A confirmed purchase shows the same floating cost receipt as fish, with XP only when earned. The tick and red cross use the same 64-unit square size and scaling as Food and Rehome. They follow the preview, preferring the space below it and moving beside or above it near screen edges. Invisible touch padding keeps the targets easy to tap and separate. The preview uses the same identity, dimensions and depth as the saved item. The tank stays sharp throughout placement. A gold and ivory outline marks the preview at its actual depth; overlapping objects in front become translucent. Bottom-anchor height controls draw order: a higher base appears behind a lower base. Horizontal position and size do not change this order. Perspective sizing remains separate. The regular HUD is hidden during placement so its controls cannot intercept positioning.

The red cross, Escape, F and S discard the preview without charging. Enter confirms. Confirmation requires a complete tap; dragging over the tick, cancelled touches, focus loss and repeated releases cannot buy an item. A release outside the tank restores the preview's previous position. Domain checks run again on confirmation. Currency shortages use the shared funds dialog. Save failures retain the preview for retry and leave the wallet and ownership unchanged.

Tap a placed item to select it, then drag it to edit its position. Repeated overlap taps cycle through covered items from front to back. The selected item keeps ownership of its drag even through translucent decorations in front. Dragging retains the grab offset, size and flip. Each valid drop saves through `MoveDecor` for the existing copy at no cost. The tick, Enter or an outside tap only ends editing; it does not write to the save. Escape and tool shortcuts also keep completed drops. The red trash icon uses `StoreDecor` to return the selected item to inventory. A failed drop returns to the saved position and keeps the selection available for another drag. Failed removal keeps the item selected so the button can retry. Neither action shows a purchase receipt.

The 64-unit four-arrow button sits directly above Sell and opens Layout mode. `hud_layout_editor.cpp` owns its input and hides the regular HUD while editing. Fish cannot intercept a decoration drag. Repeated taps on an overlap cycle through its items, while dragging keeps the current selection. Tapping another item selects it directly. A short hint explains this behavior, and small finger movements do not save an accidental move. Remove and tick form one compact pair beside the selected item, with four screen points between icons and distinct touch targets. Remove replaces the old cross and separate bag action. A red Exit button sits at the top center of the safe area. Its visible size is 56 by 24 points, with a 12-point label and a 44-point-high touch target. Exit returns to play without changing the saved position. Escape clears the selection first, then exits the mode. Size, flip and ownership are preserved. Use `--fixture shop --ui-variant layout-editor` or `layout-stash` for startup previews.

The aquarium's Bag button opens a centered Inventory dialog over the dimmed HUD. `layoutInventory` and `paintInventory` reuse the Shop's layout and painters with Plants and Decorations tabs, an All filter and Place actions. Inventory appears above the tabs, and the selected tab name appears above the filter. The dialog has no footer. Stored copies with the same catalog ID form one card with a quantity badge such as ×3; scrolling and card selection use stacks. Copies from other tanks and copies with different saved sizes or flips share the same stack while retaining their own identity and appearance. X, Escape or a full tap outside the dialog closes it; input does not reach the aquarium underneath.

Choosing Place on a stack saves one copy into the active tank through `RestoreDecor`, then selects it using the same editing controls as an already placed item. The remaining count beside Done excludes that placed copy. Each completed drag saves its new position. The tick, Enter, Done, Escape or an outside tap ends editing and keeps the item there; it does not select or restore another copy. Remove returns the selected copy to inventory. A full tank or failed restore keeps Inventory open and leaves the stack intact for retry. The flow preserves existing copy identities and uses the current save format.

Preview purchases with `--fixture shop --ui-variant plant-placement` or `decor-placement`. Add `-receipt` to either variant to show the confirmed purchase feedback. `aquarium_hud_decor_placement_tests` checks mouse and touch selection, dragging, cancellation, safe areas, purchase blockers, exact currency charges, free moves, save rollback and reloads.

### Sea-glass shop theme

`src/shop_theme.cpp` draws the deep teal header, ivory content area, mint footer, aqua cards and beveled buttons. `Canvas::gradient` and `Canvas::wave` provide scalable color fills and soft water highlights. Bubbles are also drawn in code. The layout, scrolling and shared currency HUD keep their existing dimensions and behavior. Tank uses the same themed footer and close button.

The four `assets/hud-icons/shop-tab-*-v1.png` illustrations are transparent RGBA images. Their visible alpha bounds control fitting, so the art has consistent padding without cropping or changing the original files. Labels stay in code. Generation prompts and source paths are recorded in `assets/hud-icons/generation-shop-tabs-v1.json`.

## Tank switcher and Shop tab

The original 144-unit Tank button opens a floating porthole switcher above it. All six tanks keep fixed positions in three columns and two rows, with cream frames and live tank numbers. The My Tanks header shows the number owned out of six. The active tank has a gold rim and a Current badge. Other owned tanks switch directly. Unowned tanks have dimmed artwork and a lock; tapping one opens Shop's Tanks tab with the matching card outlined in gold. Tank 6 also says Coming soon. Opening its card does not offer a purchase.

Tap Tank again to collapse, tap outside, or press Escape. Number keys 1 to 6 perform the same action as tapping the matching tank. Dragging or losing focus cancels a pending selection. Other HUD buttons remain one click away. Opening a locked tank's Shop card does not switch tanks, spend currency or change the save.

The shop tabs are Fish, Plants, Decorations, Tanks, Treasure and Backgrounds. Tanks keeps its wider cards and uses the same shop frame, card surfaces, text sizes, insets, gaps, price styling and bottom scrollbar as Fish. A single selected All tab sits in the shared filter row; tapping it resets tank browsing. Card height and top edge match the other catalog tabs. A shared clipping area keeps scrolling cards inside the surrounding frame.

Each tank card shows its name and status, fish capacity at the top, an aquarium preview, and an Upgrade or Unlock action below. The current tank has a gold Current badge and a thin gold border. Tap anywhere on an available card to open a confirmation dialog showing the new capacity and separate Coins and Pearls buttons. Only a payment button completes the purchase. Close, Escape or a backdrop tap dismisses the dialog for free. Locked, fully upgraded and Coming soon cards do not offer a purchase.

Landscape shows three complete cards and part of the next; portrait shows one large card and part of the next. Drag or swipe the row, use the mouse wheel, drag the scrollbar, or use Left, Right, Home and End to browse. Off-screen cards cannot be clicked, and dragging does not open confirmation. Opening a locked tank from the switcher scrolls its complete card into view. The dialog blocks the shop behind it, and its payment and close buttons remain at least 44 points at the checked screen sizes. Failed saves keep the dialog open with an error and leave the wallet unchanged.

Tank 1 remains the free, upgradeable starter. Only new tanks show a required level. Owned tanks offer their next five-fish capacity step at any level, up to 40 fish. Prerequisites, capacity and the selected currency still apply through Session. Tank 6 shows Coming soon and has no purchase action until the catalog defines its prices and unlock level. The Tank full message opens this tab directly. A currency shortage keeps the tank scroll position when visiting Treasure and returning. The original card mockup is recorded in [the card layout review](../evidence/tank-shop-cards/design-qa.md); the later Fish shop alignment is recorded in [the consistency review](../evidence/tank-shop-consistency/design-qa.md).

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

Keep a `DialogPaint` scope alive around each dialog frame and its contents to give them the shared 600 ms bounce. The scope draws the still backdrop; use `DialogPresentation::ModalContent` for the frame inside it. Advance the dialog's `DialogMotion` while it is open and pass the Reduce motion preference. `dialogEvent` maps pointer input to the visible pose; custom controls use `DialogMotion::inputPoint` for their own hit checks. Measure drag distances in screen coordinates so the entrance itself cannot turn a tap into a drag.

Preview examples:

```sh
./Run\ Aquarium.command --fixture aquarium --hud-dialog small
./Run\ Aquarium.command --fixture aquarium --hud-dialog custom --dialog-width 800 --dialog-height 480 --dialog-title "Tank details"
```

The full-screen Shop and floating Tank switcher use their own layouts alongside these reusable modal dialogs.

Coin and pearl shortages use the shared prompt in `hud_funds.cpp`. Fish selection, repeated egg placement, tank purchases and upgrades, and background purchases show the exact missing amount above an octopus illustration and an **Open Shop** button below. The number is gold for coins and purple for pearls. The octopus holds the matching currency. The blue header says **Not enough coins** or **Not enough pearls**, or **Not enough coins and pearls** when both are needed. There is no shop hint sentence. The button opens Treasure on the matching Coins or Pearls tab. Closing Treasure returns to the previous catalog, filter and scroll position, or to the aquarium when the purchase started there. Failed purchases do not charge the wallet. A shortage during egg placement ends placement before opening the prompt.

The prompt uses the current dialog frame and generated underwater artwork. Text and buttons remain native controls. The scene scales down when larger touch targets need more room. X, Escape and the backdrop dismiss it. Enter or Space opens the shop. Mouse and touch share the same targets; a drag or interrupted press cannot open the shop. The X and shop button retain a full touch target on phone screens. Preview the prompts with `--fixture shop --ui-variant not-enough-coins` or `--fixture shop --ui-variant not-enough-pearls`. `aquarium_hud_funds_tests` checks purchase routes, exact amounts, navigation, cancellation and responsive layout.

Bag opens the Inventory dialog with stored plants and decorations in the Shop's layout. Projects opens the large 1280 by 768 placeholder dialog and closes via X or Escape.

## Generated HUD artwork

Layout uses `assets/hud-icons/layout-button-v1.png`: four teal arrows in the same cream-and-gold frame as the other tools. Its visible alpha bounds fit the 64-unit tile. The generated PNG is copied without image edits; `assets/hud-icons/generation-layout-v1.json` records the built-in ImageGen prompt, source, alpha bounds and hash. The compact stash action reuses the Bag artwork.

Shop uses the approved thin-border `assets/hud-icons/shop-button-v3.png` with true generated alpha and a static rim highlight. Tank uses matching `tank-button-v3.png`, fitted by its visible alpha bounds to the original 144-unit button. Rewards uses matching icon-only `rewards-button-v3.png`, fitted to its original 64-unit bounds. Food uses matching icon-only `food-button-v3.png`, fitted to its original 64-unit bounds. Bag and Projects use matching icon-only v3 buttons fitted to their original 108-unit bounds. Settings retains its existing artwork. The border highlight is not animated.

Coin and Pearl use transparent v4 cartoon currency sprites, fitted to their existing HUD and menu wallet bounds. Bar sizes and overlaps remain unchanged.

Shop footer wallets share the main HUD layout and renderer, translated to the footer with plus buttons hidden. Rewards uses the shared dialog frame with the pearl milestone, a progress bar and the eligible-sale rule. Its layout accounts for window points so labels and the close target remain usable on phones, including portrait. Pearls are credited automatically and fly to the pearl counter with the sale's other rewards. Reduced motion uses immediate totals and a highlight. X or Escape closes the dialog. Settings uses the same frame and header around its four turquoise cards and amber controls. See [Settings](settings.md) for behavior, saved preferences and validation.

The sell reward ribbon uses `assets/hud-icons/sell-reward-ribbon-v1.png`, fitted to its recorded visible bounds. Its coin medallion and parchment/teal artwork are generated; the coin amount and XP are live text. At the user's request, the ribbon now renders at half the original width and height, including its text, icons, outlines and spacing. The minimum label width on narrow screens is about 44 pixels. Done reuses the same Buy surface and white label as feeding and egg placement.
