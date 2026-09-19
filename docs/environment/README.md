# Complete aquarium backgrounds

Open Shop > Backgrounds. Open Blue Cove is the free default. Sage Lagoon is a complete alternative for 1,200 coins. The Owned filter shows backgrounds available to use for free.

Each tank selects one background. Buying a background permanently unlocks it for every tank without changing the current scene. Selecting an owned background applies it for free. Both actions keep the shop open. Backgrounds use no decoration slots and award no XP. Fish and placed decorations remain separate. Backgrounds have no glass overlay.

## Artwork

- `assets/environment/open-blue-cove-v1.png`: the selected second mock, with blue rock formations, a distant left arch, simple seaweed silhouettes, open aqua water and pale sand.
- `assets/environment/sage-lagoon-v1.png`: the selected third mock, with sage and blue-green rocks, loose leaf silhouettes, mint-aqua water and broad cream sand patches.

Both are complete opaque 1672 by 940 images made with built-in Image Gen. The user selected [mockups 2 and 3](../../design/mockups/background-directions-2026-09-18/README.md) on 18 September 2026. Background extraction removed the mock interface, fish and placed objects, then reconstructed the scenery behind them. Exact prompts, source references, dimensions and hashes are in the adjacent `.provenance.json` files and `assets/manifest.json`.

The earlier Sunlit Lagoon and Coral Garden PNGs remain as source history and are no longer selected by the catalog. Their prompts remain in `background-prompts.md`. The earlier layer experiment is archived in `design/environment-layer-study`; its prompts remain in `art-prompts.md`.

The renderer draws the selected background once, then gameplay. Backgrounds and placed plants and decorations share the same uniformly scaled, bottom-anchored scene. A saved anchor refers to the same point in the artwork on every device, and sprite size, motion and selection glow scale with that artwork. Wider phones crop the top; tablets crop the sides, including objects attached to those parts of the scene. Rendering and resizing never move saved anchors to compensate for a crop. Shop previews use the same artwork as the tank.

Dragging, placement controls, hit testing and purchase feedback use the same scene coordinates. New drops fit the item's footprint inside the current visible crop. Swimming fish and their controls continue to use the full visible water area. Safe insets affect controls, without shifting the scene or saved decor. Background assets must retain the 1672:940 aspect ratio; the rendering test checks this contract.

### Background tone

Backgrounds use broad cartoon shapes, few shade planes and simple pale sand so fish, plants and placed decorations stand out. The [FishVille reference screenshot](https://www.mobygames.com/game/52953/fishville/screenshots/browser/525489/) informed this separation between the scenery and placed objects.

The earlier saturation, contrast and brightness grade and forced quarter-size sampling have been removed. The renderer displays the approved colors and drawing style with normal cached texture sampling. Fish, plants, placed decorations and interface artwork keep their existing rendering.

To add a background, add one PNG and an entry in `environmentCatalog()` in `src/environment.cpp`. Give it a stable ID, name, coin price, and asset path. Keep the center clear for fish, and keep important scenery away from the extreme edges. The shop price is an initial tuning value, not a real-money purchase.

## Saved state

`Tank::backgroundId` stores the selected background. `State::environmentOwned` stores permanent ownership. The saved IDs remain stable: `sunlit-lagoon` now displays Open Blue Cove, and `coral-garden` now displays Sage Lagoon. Existing purchases and tank selections carry over. New tanks and older v4 saves without background fields receive Open Blue Cove.

The earlier reef/sand save fields are read only for migration. Any paid Coral Arch or Pearl Sand purchase grants the paid background, now Sage Lagoon, without another charge. If a paid old style was equipped, that tank receives Sage Lagoon; otherwise it receives the default. Migration preserves balances, receipts, and ledger records. Multiple old purchases map to one permanent unlock. New saves write only `backgroundId`, not separate reef or sand choices.

Purchases and selection changes use the existing atomic transaction path, receipts, ledger, and save rollback. Repeated purchase taps cannot charge for an owned background. Unknown or unowned equipped backgrounds are rejected on load.

## Run and verify

```sh
cmake --build --preset desktop
./build/desktop/aquarium --assets assets
ctest --test-dir build/desktop -R 'environment_contract|environment_rendering|clay_hud_layout' --output-on-failure
```

The domain test covers purchases, duplicate requests, save failures, real save/reload, per-tank selection, and old-save migration. The rendering test checks both complete backgrounds at desktop, tablet, and wide-phone sizes and captures the background shop. It also renders the same saved layout at 1338 by 752, 852 by 393, 1024 by 768, 1180 by 820 and 667 by 375, including 2x and 3x density and phone safe insets. It checks artwork-relative anchors and sizes, inverse input, placement at the same artwork landmark, save reload, edge placement, and unchanged saves after resize. Captures are written to `build/desktop/environment-captures`. Pass `--native` as the test executable's third argument after the asset and output paths to repeat the saved-layout checks with native rendering.

Alignment follow-up on 2026-09-18: desktop and iOS simulator builds and six relevant suites passed. Both backgrounds were checked with the same saved decorations at phone and tablet sizes using software and native rendering. See the [alignment review](../../evidence/placement-alignment-2026-09-18/design-qa.md).

Verified on 2026-09-18: the desktop build and five relevant suites passed for the selected artwork: background transactions, background rendering, native rendering quality, HUD layout and menu loading. Native desktop, phone and tablet previews were compared with the selected mocks. The [review and captures](../../evidence/backgrounds-2026-09-18/design-qa.md) record the evidence and limits.

Verified on 2026-09-16: desktop build and 11 relevant tests passed, including background transactions and rendering, Clay HUD and tank menus, storage, lifecycle, and decoration rules. Both scenes were visually checked at 1338 by 752, 1024 by 768, and 852 by 393.

`--scene-only` retains its blank-art behavior for layout review. `--hud-layout` shows the selected complete background.
