# Fishius: shop, dialogs and game screens

Eight mockups extend the approved fullscreen shop style. The shop's fifth tab is now **Coins & Pearls**, replacing **Food**. The separate Food tool in the aquarium still provides free feeding.

Created on 15 September 2026. These are visual design proposals. Runtime screens and font settings have not been changed.

## Mockups

| Screen | Main design decision |
| --- | --- |
| [01. Coins & Pearls shop](/Users/amoghrijal/amogh/aquarium-native/design/mockups/fishius-ui-kit-2026-09-15/01-coins-and-pearls-shop.png) | Fullscreen catalog with six currency packs and one Starter Bundle. |
| [02. Settings](/Users/amoghrijal/amogh/aquarium-native/design/mockups/fishius-ui-kit-2026-09-15/02-settings.png) | Six clear tiles for sound, reduced motion, music availability, language and local help. |
| [03. Not enough coins](/Users/amoghrijal/amogh/aquarium-native/design/mockups/fishius-ui-kit-2026-09-15/03-not-enough-coins.png) | Exact shortfall, cost, current balance and Open Shop action. |
| [04. Rehome confirmation](/Users/amoghrijal/amogh/aquarium-native/design/mockups/fishius-ui-kit-2026-09-15/04-rehome-confirmation.png) | Clear consequence and one-time adult reward, with Back and Rehome actions. |
| [05. Tanks](/Users/amoghrijal/amogh/aquarium-native/design/mockups/fishius-ui-kit-2026-09-15/05-tanks.png) | Five tank rows, locked levels, separate growing and display capacity, and alternative upgrade prices. |
| [06. Collection & Mastery](/Users/amoghrijal/amogh/aquarium-native/design/mockups/fishius-ui-kit-2026-09-15/06-collection-and-mastery.png) | Current visible catalog, four adult milestones and display companions without mastery stars. |
| [07. Bag](/Users/amoghrijal/amogh/aquarium-native/design/mockups/fishius-ui-kit-2026-09-15/07-bag.png) | Stored items can be placed for free; Arrange Tank opens the move/store flow. |
| [08. Level-up](/Users/amoghrijal/amogh/aquarium-native/design/mockups/fishius-ui-kit-2026-09-15/08-level-up.png) | Level 2 reward and four real newly available items, with one Continue action. |

All images retain their native 1672 × 941 pixel output.

## Free fonts

[View the actual font comparison](/Users/amoghrijal/amogh/aquarium-native/design/mockups/fishius-ui-kit-2026-09-15/09-free-fonts.png).

| Font | Use | Local file | Licence |
| --- | --- | --- | --- |
| Luckiest Guy | Large headings and celebrations. My closest free heading choice among the bundled fonts. | [TTF](/Users/amoghrijal/amogh/aquarium-native/assets/fonts/LuckiestGuy-Regular.ttf) | [Apache 2.0](/Users/amoghrijal/amogh/aquarium-native/assets/fonts/LuckiestGuy-LICENSE.txt) |
| Lilita One | Buttons, fish names, prices and mixed-case labels. | [TTF](/Users/amoghrijal/amogh/aquarium-native/assets/fonts/LilitaOne-Regular.ttf) | [SIL OFL 1.1](/Users/amoghrijal/amogh/aquarium-native/assets/fonts/LilitaOne-OFL.txt) |
| Nunito SemiBold | Descriptions, settings and smaller statistics. Select weight 600 from the variable font. | [TTF](/Users/amoghrijal/amogh/aquarium-native/assets/fonts/Nunito-Variable.ttf) | [SIL OFL 1.1](/Users/amoghrijal/amogh/aquarium-native/assets/fonts/Nunito-OFL.txt) |

These fonts allow use and embedding in the game under their licences. Keep the licence and copyright notices with distributed font files. The source licences were checked against Google Fonts: [Luckiest Guy](https://github.com/google/fonts/blob/main/apache/luckiestguy/LICENSE.txt), [Lilita One](https://github.com/google/fonts/blob/main/ofl/lilitaone/OFL.txt), [Nunito](https://github.com/google/fonts/blob/main/ofl/nunito/OFL.txt).

The comparison PNG uses the actual project TTF files. The eight ImageGen mockups approximate the intended lettering and do not prove exact font metrics. Use the TTF sample to judge the implementable type. No custom font file was made and no paid font is required.

## Shared visual rules

| Element | Direction |
| --- | --- |
| Main frame and text outline | Deep navy, target #07364B. |
| Header and grid background | Dark petrol, target #072D40. |
| Panel body and selected tabs | Warm ivory, target #FFF7E6. |
| Cards and secondary actions | Aqua, target #27C7E9. |
| Main action | Bright green, target #5AE523. |
| Close and rehome confirmation | Coral red. |
| Wallet and footer | Teal. |
| Display text | White or cream fill, navy outline and short downward shadow. |
| Small text | Plain dark navy on light surfaces; no heavy outline. |

Use the same red square X, rounded card corners, top highlight and lower bevel throughout. Fullscreen browsers use a fixed header and footer. Dialogs use a cream body over a dimmed aquarium.

For a 48 px display label, start with a 2–3 px navy outline and a 3–5 px downward shadow. Tune by viewing at the actual device size. Start with a 44 point minimum touch area. These are implementation targets, not pixel measurements extracted from generated images.

## Game data used

The current working tree and [catalog snapshot](/Users/amoghrijal/amogh/aquarium-native/design/mockups/fishius-ui-kit-2026-09-15/catalog-snapshot.json) take precedence over older screenshots.

### Currency offers

| Offer | Amount | Preview price |
| --- | --- | --- |
| Small Coin Pack | 2,500 coins | $0.99 |
| Medium Coin Pack | 12,000 coins | $3.99 |
| Large Coin Pack | 35,000 coins | $9.99 |
| Small Pearl Pack | 15 pearls | $1.99 |
| Medium Pearl Pack | 40 pearls | $4.99 |
| Large Pearl Pack | 90 pearls | $9.99 |
| Starter Bundle | 25,000 coins + 30 pearls | $4.99 |

These are the existing preview offers in [currency_shop.cpp](/Users/amoghrijal/amogh/aquarium-native/src/currency_shop.cpp). Purchases are not connected in the current game. The mockup shows “Purchases coming soon”.

### Dialogs and progression

- **Shortfall:** Tank 1 expansion costs 300 coins. A 250-coin wallet is 50 coins short.
- **Tank expansion:** At level 3, Tank 1 can expand from 10 to 15 growing slots for 300 coins **or** 3 pearls. Its eight display slots remain separate.
- **Tank unlock levels:** Tanks 2, 3, 4 and 5 become available at levels 7, 16, 25 and 34. Later tanks require the preceding tank.
- **Rehome:** The adult Platy was bought at level 1, so its stored quote is 375 coins and 80 XP. Keep and Rehome give the same one-time reward. Keep retains a display fish; Rehome removes it.
- **Collection:** The current catalog filter yields 36 launch species with ready art at level 40 or below. The old screenshot shows 46. This mockup follows the current filter and shows four discovered species.
- **Mastery:** Adult milestones are 1, 3, 10 and 25. Premium display companions have no adult mastery. Rewards remain marked as coming soon.
- **Bag:** The four sample decorations are already owned and stored, so placing them is free. No bag capacity limit is implied.
- **Level 2:** The current catalog reward is **500 coins and one pearl**. The older screenshot's 100 coins was not used. Newly available items are Zebra Danio, Bubble Eye Goldfish, Dwarf Hairgrass Patch and Spiral Shell.
- **Settings:** Sound, volume and reduced motion have persisted state. Music is unavailable; English is the current language. Open Guide and FAQ refer to local help.

Balances and background HUD counters are illustrative screen states. Bind them to live game data during implementation. The shop, upgrade, shortfall, adult reward and level-up figures above were checked against current source.

## Sources and review

Design references: [Clash of Clans gallery](https://interfaceingame.com/games/clash-of-clans/), [shop](https://interfaceingame.com/screenshots/clash-of-clans-gems/), [settings](https://interfaceingame.com/screenshots/clash-of-clans-settings/), [confirmation dialog](https://interfaceingame.com/screenshots/clash-of-clans-finish-now/).

Game sources: [content catalog](/Users/amoghrijal/amogh/aquarium-native/assets/content.json), [currency offers](/Users/amoghrijal/amogh/aquarium-native/src/currency_shop.cpp), [settings and collection](/Users/amoghrijal/amogh/aquarium-native/src/skin.cpp), [tank behavior](/Users/amoghrijal/amogh/aquarium-native/src/tanks.cpp), [adult details](/Users/amoghrijal/amogh/aquarium-native/src/details.cpp), [Bag](/Users/amoghrijal/amogh/aquarium-native/src/decor_view.cpp), [level-up](/Users/amoghrijal/amogh/aquarium-native/src/level_up.cpp), [level rewards](/Users/amoghrijal/amogh/aquarium-native/src/domain.cpp).

Each mockup was visually checked for legible main copy, complete panel/card bounds, shared styling and the main source-backed figures. The collection received a targeted edit to remove mastery stars from its two display companions. The typography specimen was rendered with actual font files and inspected.

No code tests were needed for these static artifacts. The game was not rebuilt or pushed.

## Reproduction

- [Exact image prompts](/Users/amoghrijal/amogh/aquarium-native/design/mockups/fishius-ui-kit-2026-09-15/prompts.md)
- [Generation provenance and ordered reference images](/Users/amoghrijal/amogh/aquarium-native/design/mockups/fishius-ui-kit-2026-09-15/provenance.json)
- [Font specimen renderer](/Users/amoghrijal/amogh/aquarium-native/design/mockups/fishius-ui-kit-2026-09-15/render-font-samples.py)
