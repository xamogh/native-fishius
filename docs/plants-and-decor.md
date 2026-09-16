# Plants and decorations

Current economy and lifecycle rules are in [v4 implementation, steps 1 to 3](workbook-implementation.md). That record supersedes older prices, purchase XP, care penalties, capacity and save compatibility statements below.

The source is the supplied `Aquarium_Game_Master_With_Plants_and_Decor.xlsx`, copied byte for byte to `design/aquarium_game_master_model.xlsx`. Its Decor Catalog, Decor Art Briefs, Decor Guide and Decor Live Ops sheets control the implementation. The existing fish catalog and XP curve are unchanged.

The source catalog has 120 items: 60 plants and 60 decorations. It contains 64 coin items, 32 premium items, and 24 limited items arranged in 12 coin/pearl event pairs. The Shop always shows the 36 regular plants and 36 regular decorations with unlock levels 1 through 40. It also shows seasonal items while their configured events are active. Items above level 40 are hidden and cannot be purchased. Source records and saved ownership are retained for all hidden items.

Fish, Plants and Decorations show four items per page on phones and wide desktop viewports. iPad and taller tablet viewports show eight items in four columns and two rows. All three catalogs sort by unlock level from lowest to highest, including across pages. Coin and premium editions do not move lower-level items behind higher-level items. The catalogs share card spacing, typography, currency icons, price buttons, lock indicators and paging controls. Plants and Decorations browse the level 1 through 40 catalog, including active configured event items, without filters. Bag provides stored items and the Arrange tank control.

## Purchases and ownership

Selecting a plant or decoration in Shop opens a temporary preview at the centre-front of the tank. It does not charge currency, grant XP or create an owned copy. Drag the preview to position it, or tap a new position. The grabbed point stays under the pointer or finger, and release leaves the preview in place. A green footprint and arrows mark the item. Nearby red X and green tick buttons stay within the screen's safe area, with at least 44-point touch targets. The price appears below them.

The green tick buys and places the item in one command. Currency, level, event availability and collection limits are checked again before the purchase. A successful confirmation charges once, grants the applicable first-ownership XP, records ownership and advances Tank Stylist. Repeat taps cannot buy another copy. If confirmation fails, the preview stays available with the existing funds dialog or an error message. The red X, Escape, a tool change or opening a menu cancels an unpurchased preview without changing currency, XP or ownership.

Tool modes use a small Done button without an instruction banner. Placement has no bottom hint banner. During placement or movement, the aquarium, preview and placement controls remain visible while the normal HUD is hidden. Storage warnings still appear when progress cannot be saved. New previews are not saved because they contain no purchased items.

Fish purchases queue paid eggs. In egg placement mode, each water tap drops the oldest egg, clamped to 5–95% of the water width and 5–60% of its height. Capacity is checked on drop. A full tank keeps the egg queued for an available tank. Tool changes keep the egg queue and let the player use fish selection, Food and Sell. The player can resume egg placement when ready. Hatching starts on drop and takes six seconds. Eggs sink at individually varied rates to 93% of the water height.

The native save retains paid eggs and supports pending decor from older saves. Confirming an older paid copy places it without another charge. Cancelling it stores the copy in Bag.

Each copy has a persistent identity, one tank assignment, a position, a size multiplier, a stored flag, and a flip flag. Enter decorate mode with Arrange tank, then tap a plant or decoration to select it. Normal Select mode targets fish only. Selection adds an amber highlight, its name, and a nearby Stash button. Drag the selected item to preview a move, then press the tick to save its position. The X leaves its original position unchanged. Restoring a stored copy uses the same confirmation controls and keeps the copy stored until confirmed. A small finger movement still counts as a tap. An interrupted drag or a release outside the tank leaves the saved position unchanged. Stash stores the selected copy, and the Bag button opens stored items. Moving, storage and restoration are free. Existing saved sizes and flips are preserved. A stored copy can be placed in any owned tank. Fish retain selection priority where they overlap decor.

The catalog XP is granted only on the first paid acquisition of an item ID. The lifetime ownership record survives storage, transfers, duplicate purchases, reloads, and event reruns. Cosmetics have no sale or conversion action and no fish-stat or capacity modifiers.

A tank's decor score sums the workbook scores for distinct item IDs currently placed there. Stored copies contribute nothing. Extra copies of an ID in the same tank contribute nothing. Scores are calculated from placement, not incremented as quest or purchase rewards. The Tank Stylist objective can be completed by placing, moving, resizing, flipping, or restoring an owned item without spending currency. Opening or cancelling a preview does not complete it; unchanged moves and unchanged sizes do not count as rearrangement.

The placement limit is 32 copies per tank, separate from fish slots. Previously saved copies are preserved when loading, even if an older save exceeds the new placement limit. Further placements require room. The existing 500-copy save safety bound remains in place.

## Catalog access and compatibility

All 36 regular plants and 36 regular decorations with unlock levels up to 40 are visible from the start, including in existing saves that have not placed a plant. There is no first-plant requirement. Purchases still follow each item's level and currency rules. Event items appear while their configured event is active, including level-locked cards. The domain still checks level, price and event availability when a purchase is made. The older full tutorial still contains deferred gift and reward steps.

The optional first CP-01 Pebble Moss Tuft purchase still costs 25 coins and grants 15 XP in total, including its existing 14 XP bonus. Other items can be bought first. Each shop card shows the XP its purchase will grant. Existing bonus claims, balances, fish and decorations are preserved. Saves that predate this bonus receive no retroactive XP.

The earlier seaweed, coral, shell, arch and chest IDs are not in the updated workbook. Existing copies keep their original identity and artwork and remain movable and storable. They are not resold in the new catalog and add no new workbook score. No catalog item is silently substituted for an older item.

## Event configuration

The workbook describes proposed windows and requires an active configured event. Its proposed dates are retained as source metadata. They are not automatically activated. Lunar New Year and the anniversary require yearly scheduling.

The runtime accepts an optional `assets/decor-events.json` containing an array of event records with `name`, `starts_at`, and `ends_at`. Timestamps are UTC Unix milliseconds. The start is inclusive and the end is exclusive. A live-ops service can supply this configuration. This local application does not add a new backend or invent an event calendar.

Only the 12 workbook event names are accepted. Configuration validation is atomic. Event configuration changes acquisition only; stored or placed event items and their animations remain usable after an event closes.

## Artwork and motion

Every item uses its ID to join its catalog row, art brief, sprite and icon. Generated masters live under `design/decor-art/masters`; runtime sprites and icons live under `assets/decor`. Prompt records are in `assets/decor/provenance`.

The hourglass and snowglobe have separate empty-glass animation plates. The pinwheel has a separate base and rotating blade layer with a fixed hub. The original catalog sprite remains the reduced-motion pose. The glass patches preserve the original silhouette and alpha; their source records and preparation script are retained.

Art retains its workbook dimensions and bottom-centre anchor. Plants and decorations can be placed within the full visible tank, including the foreground floor. Placement considers the scaled footprint within the world bounds, x 0–1088 and y 0–635, so an item cannot disappear above or beside the tank. If an item is larger than the tank, it stays bottom-aligned and reachable. Purchases, previews, moves, resizing and restoration use the same rule. Loading older saves also corrects out-of-view positions, including the five legacy items, without changing ownership. The Fishium perspective remains: depth is `pow(ny, 0.82) * (1 - 0.44 * pow(edge, 1.6))`, with `ny` clamped from y 153.6–504.32. Positions beyond that depth range retain the nearest depth endpoint without snapping their anchor. Scale spans 0.42–1.18, multiplied by the saved manual size. A screen-blended #5fc4dc silhouette provides haze up to 0.26 at the back. The initial preview remains at world (544, 441.1904), uses 0.92 opacity and shares the placed item's live depth calculation.

Plants and decorations, including a moving preview, render in workbook layer order: Background, Midground, then Foreground. Items within each layer sort by bottom anchor, preserving their existing order when anchors match. Decor hit-testing uses that same order in reverse. Fish, eggs and food render in front, and fish keep selection priority. The placed item keeps the preview size immediately, with no placement pop or scale tween. Preview copies also use the future placed identity so animated poses stay continuous. Runtime plant/decor motion still uses the authored local geometry, grouped motion and restricted internal or bubble effects. Ordinary coin items remain static.

At most six placed items animate at once. At most two external bubble emitters are active, with a shared six-particle budget. The independent Reduced Motion switch in Settings uses the still sprite and disables decor animation and effects. Its value is saved and restored. Event availability never disables an owned item's motion.

## Verification commands

Run the content import, the independent workbook checks, the importer regression tests, the domain tests and the native UI tests from the repository root. `tools/check_decor_catalog.py` independently reads the workbook; it does not use the importer as proof. `tools/prepare_decor_art.cjs` creates the master/runtime/icon derivatives, alpha audit and labelled contact sheets.

The art preparation scripts require Sharp, resolved from `SHARP_MODULE` or the normal Node module path. `tools/prepare_decor_layers.cjs` rebuilds the two glass plates from the retained generated-source paths in the provenance manifest. The checked-in masters allow ordinary runtime and icon exports without those external paths.

Verification results are recorded in `evidence/decor-verification.json`. UI captures cover 667 by 375, 852 by 393, and 1024 by 768 viewports, plus animated, flipped, and reduced-motion premium fixtures. `evidence/decor-ui/native-scene.png` records the native Metal preview.

Physical-device frame timing and the workbook's unspecified title/background rewards remain separate from these catalog and placement rules.
