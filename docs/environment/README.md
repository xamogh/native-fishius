# Complete aquarium backgrounds

Open Shop > Backgrounds. Sunlit Lagoon is the free default. Coral Garden is a complete alternative for 1,200 coins. The Owned filter shows backgrounds available to use for free.

Each tank selects one background. Buying a background permanently unlocks it for every tank without changing the current scene. Selecting an owned background applies it for free. Both actions keep the shop open. Backgrounds use no decoration slots and award no XP. Fish and placed decorations remain separate. Backgrounds have no glass overlay.

## Artwork

- `assets/environment/sunlit-lagoon.png`: turquoise water, blue-gray rock formations, a stone arch, lavender coral, and golden sand, based on the user's reference.
- `assets/environment/coral-garden.png`: a richer coral garden with pink branches, lavender sea fans, orange sponges, and an open center for fish.

Both are complete opaque 1672 by 941 images generated with the built-in image generation tool. Exact prompts are in `background-prompts.md`. The earlier layer experiment is archived in `design/environment-layer-study`; it is not sold or used by the renderer. Its original prompts remain in `art-prompts.md`.

The renderer draws the selected PNG once, then gameplay. Uniform cover scaling preserves the image's proportions. Bottom anchoring keeps the sand visible on wide phones; tablets crop some scenery at the sides. Shop thumbnails show the complete scene in its original proportions.

To add a background, add one PNG and an entry in `environmentCatalog()` in `src/environment.cpp`. Give it a stable ID, name, coin price, and asset path. Keep the center clear for fish, and keep important scenery away from the extreme edges. The shop price is an initial tuning value, not a real-money purchase.

## Saved state

`Tank::backgroundId` stores the selected background. `State::environmentOwned` stores permanent ownership. New tanks and older v4 saves without background fields receive Sunlit Lagoon.

The earlier reef/sand save fields are read only for migration. Any paid Coral Arch or Pearl Sand purchase grants Coral Garden without another charge. If a paid old style was equipped, that tank receives Coral Garden; otherwise it receives the new default. Migration preserves balances, receipts, and ledger records. Multiple old purchases map to one permanent unlock. New saves write only `backgroundId`, not separate reef or sand choices.

Purchases and selection changes use the existing atomic transaction path, receipts, ledger, and save rollback. Repeated purchase taps cannot charge for an owned background. Unknown or unowned equipped backgrounds are rejected on load.

## Run and verify

```sh
cmake --build --preset desktop
./build/desktop/aquarium --assets assets --hud-layout
ctest --test-dir build/desktop -R 'environment_contract|environment_rendering|clay_hud_layout' --output-on-failure
```

The domain test covers purchases, duplicate requests, save failures, real save/reload, per-tank selection, and old-save migration. The rendering test checks both complete backgrounds at desktop, tablet, and wide-phone sizes and captures the background shop. Captures are written to `build/desktop/environment-captures`.

Verified on 2026-09-16: desktop build and 11 relevant tests passed, including background transactions and rendering, Clay HUD and tank menus, storage, lifecycle, and decoration rules. Both scenes were visually checked at 1338 by 752, 1024 by 768, and 852 by 393.

`--scene-only` retains its blank-art behavior for layout review. `--hud-layout` shows the selected complete background.

The older UI still depends on missing legacy assets and fixtures in this checkout, including `skin/icon-star.png`, `tank-menu/panel.png`, and `general-dialog/close.png`. Use the Clay HUD command above to review the working background shop.
