# Tank shop and switcher

Selected for implementation: [the larger porthole switcher](porthole-switcher-approved-large.png). The user selected the original larger version, rather than the later smaller refinement. Native implementation and comparison captures are documented in [the QA review](../../../evidence/tank-switcher/design-qa.md). The current catalog has five tanks; Tank 6 appears as Coming soon in Shop, as requested. Earlier concepts remain below.

Earlier revision: [sea-glass switcher](seaglass-switcher-v1.png), with the current Tank icon design, aqua aquarium artwork, cream number tabs and deep-blue backing from the existing Shop palette. [Exact Image Gen prompt](seaglass-switcher-prompt.md). Earlier variations are retained below for comparison.

[Generated mockup](tank-shop-and-switcher-v1.png) | [Exact prompt and references](prompt.md)

One concept with two views, generated using built-in Image Gen on 17 September 2026. The user requested a mockup and confirmed six tanks total, with the first tank upgradable.

- Shop adds a Tanks tab to the existing five categories. Its six cards represent Tank 1 through Tank 6.
- Tank 1 is the free starter. Its next upgrade increases growing capacity from 10 to 15, for either 300 coins or 3 pearls. Display capacity stays at eight.
- Owned cards offer upgrades. Available cards offer a purchase. Locked cards show an unlock state without prices.
- My Tanks opens a small tray at the lower left. It lists owned tanks, marks the current tank and switches on a row tap. It has no buying or upgrade actions.
- The example owns two tanks. Wallet balances are illustrative. The first five tanks use current catalog values; Tank 6 is proposed, with its level and price unspecified.

The mockup was visually checked for six distinct tank cards, the starter upgrade, the selected Tanks tab, the owned-only switcher, and a visible aquarium. In implementation, the expanded switcher should use a downward collapse chevron and the collapsed control should use an upward chevron. The generated board shows an upward chevron in both states.

This is a design image. Game code and catalog data were not changed.

## Follow-up: starter plus five extra tanks

[Six owned tanks mockup](six-owned-tanks-v1.png) | [Exact Image Gen prompt](six-owned-tanks-prompt.md)

The user requested a mockup showing five extra tanks. This full landscape view shows six owned tanks total, including the starter. All six switcher rows are visible without scrolling. Tank 1 is marked Current, and the expanded button has a downward collapse chevron. The aquarium and right-side controls remain visible. The mockup was visually checked for those details. Player progress and wallet values are illustrative.

## Collapsed state

[Collapsed mockup](collapsed-tanks-v1.png) | [Exact Image Gen edit prompt](collapsed-tanks-prompt.md)

The collapsed state hides the entire tank tray and leaves the My Tanks (6) button at the same bottom-left position. Its upward chevron indicates that the list opens above it. The rest of the game screen keeps the expanded mockup's composition. The output was visually checked for the removed tray, restored scenery, upward chevron and consistent surrounding controls.

## Latest revision: current Tank icon and minimal numbered stack

The user asked to keep the current main-screen Tank icon and preferred the minimal FishVille reference. Built-in Image Gen received the actual tank-button-v3.png artwork, the real HUD screenshot, both user references, and the prior lagoon mockup.

The revised board shows the same Tank icon in both states. Collapsed shows only the square icon. Expanded adds six small numbered aquarium thumbnails above it, a small My Tanks (6) heading, and a gold outline on Tank 1. The numbered stack has no text rows, prices, upgrade actions or large surrounding drawer. Tank purchases and upgrades remain in Shop.

Visual review confirmed numbers 1 through 6, the original button design in both views, and a narrow selector that leaves the aquarium visible. This is a generated design mockup, not a game implementation. The original asset remains unchanged on disk.

## Follow-up: closer to the FishVille reference

The user requested a closer resemblance to FishVille. The new full-screen mockup uses its compact purple header and vertical spine, isometric blue aquarium boxes and large white numbers. The existing square Tank button remains at the base. Tank 1 has a gold selection highlight. All six tanks are owned, and the selector has no purchase or upgrade actions.

Built-in Image Gen used both supplied FishVille screenshots, the original tank-button-v3.png and the previous lagoon mockup. Visual review confirmed all six distinct numbers, angled box shapes, the current Tank button design, and preservation of the surrounding gameplay controls. No game code or catalog values were changed.

## Latest revision: follow the current game UI

The user found the FishVille-style version too similar to its reference and requested a different style closely following the existing UI. This revision uses the actual Shop screenshot, current tank-grid artwork and original Tank button as visual sources.

The six tank choices use mostly front-facing aqua aquarium art, small cream tabs numbered 1 through 6, deep-blue backing and a cream header. Tank 1 uses a gold selection rim and number tab. The original Tank button design stays at the base. The compact switching interaction, surrounding aquarium and right-side controls are retained.

The built-in Image Gen output was visually checked for six correctly numbered tanks, the new number-tab treatment, the current game palette, the selected tank and the original main button design. It is a design mockup only. Game code and catalog data were not changed.
