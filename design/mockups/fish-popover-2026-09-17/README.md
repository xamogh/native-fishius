# Compact fish popover

Mock image requested on 17 September 2026. Generated with the built-in Imagegen tool.

[View the mockup](01-compact-adult.png) · [Exact prompt](PROMPT.md)

Latest revision: [Refined rewards and palette](02-palette-and-rewards.png) · [Exact edit prompt](02-palette-and-rewards-prompt.md)

## Direction

Use a small speech bubble attached to the selected fish. Preserve the visible aquarium and existing blue and cream UI. The fish stays outside the bubble. The adult view uses a title, growth strip, shared payout, and two actions.

The supplied FishVille screenshot guides the compact size and fish attachment. The current game screenshot guides the artwork and UI style.

## Verified workbook content

Source: `design/aquarium_game_design_v4.xlsx`.

- `Coin Fish!B24:N24`: Longhorn Cowfish, Rare, unlock level 30, 44 hours, 1,260 profit, 315 purchase price, 1,575 adult payout, 230 adult XP. The mock uses an instance purchased at its unlock level.
- `Rules!D8`: Keep and Rehome pay the same adult reward once.
- `Rules!D9`: Rehome starts at Junior. Eggs and Babies cannot be sold.
- `Rules!D10`: Baby, Junior, Young, Mature and Adult thresholds are 0%, 25%, 55%, 80% and 100%.
- `UX & Accessibility!D6:E6`: One readable popover anchored to the selected fish, with useful growth and reward information.
- `UX & Accessibility!D21:E21`: Favorite protects against Rehome, which requires an explicit confirmation.

## Implemented states

- Adult: show Ready, a full growth strip, one payout and Keep / Rehome.
- Growing: show the current stage, progress, remaining fed growth time and current Rehome payout. Show the adult payout as a compact comparison. Keep becomes available at adulthood.
- Hungry: replace the time/status line with Hungry · Growth paused. Follow the existing free feeding rule.
- Egg or Baby: disable Rehome and explain that it unlocks at Junior.
- Rehome confirmation: reuse the bottom action area in the same popover.
- Favorite: protect against Rehome.
- Keep: collect once and retain the fish as a display companion. Use Bag if display slots are full.
- Reposition the bubble near screen edges so the tail still points to the fish and the controls remain visible.

These states are implemented in the native fish popover.

## Visual check

The generated image has one compact bubble with a tail pointing to the visible Cowfish. All five growth stages, the adult status, the correct payout and its breakdown, Favorite, Close, Keep and Rehome are readable. The aquarium remains bright and the main controls remain visible. The surrounding HUD uses the source screenshot's illustrative player state; the Cowfish reward uses the workbook's level 30 purchase example.

## Refined rewards and palette

The user approved the compact design and requested a better coin and XP section and a more pleasant palette. The second mockup uses an open reward row with two aligned groups, explicit Coins and XP labels, a fine divider, and a smaller return/profit breakdown. The body uses warm ivory, the Keep button uses muted jade, and Rehome uses slate teal. The XP star uses blue to distinguish it from the gold coin.

The built-in Imagegen tool edited the approved first mockup. Visual review confirmed all reward values, the five stages, both actions, and the fish pointer. The bubble remains compact and the surrounding aquarium and HUD keep their composition.

## Native implementation

Implemented in `HudCare` on 17 September 2026. The adult frame measures 469.7 × 314.5 pixels at the mockup's 1642 × 958 viewport. It can move above, below or beside the selected fish to stay within safe edges and avoid the HUD. Growing, confirmation and notice states add one compact text row. Touch controls retain a 44-point minimum.

Per the user's implementation instructions, the game keeps its existing fonts, dialog surfaces and button palettes. Coins reuse `hud-icons/coin-v4.png`; Rehome reuses `tools/sell-net.png`. XP shares the existing HUD badge renderer because the project has no standalone XP icon. Hearts are glyphs from the current body font. No new runtime image, icon or font assets are added.

Native rendering, interaction coverage, comparison history and validation limits are in [the implementation QA report](../../../evidence/fish-popover-compact/design-qa.md).
