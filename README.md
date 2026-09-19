# Fishius: a native C++20 and SDL3 aquarium game

The game uses native SDL rendering, textured fish meshes, an event-driven care model, physical food, a local wallet, tanks, fish with growth stages, favorites, inventory, and JSON saves. The supplied behavior contract is the movement and interaction reference. The supplied spreadsheet is the catalog source. No original game source or running application is required.

Open `Run Aquarium.command` to build and run the current interface with your saved game. The old interface and its Studio editor have been removed. `--hud-layout` remains a harmless alias for older launch commands; it no longer selects a different interface.

## Delivery status

Read `docs/workbook-implementation.md` for the workbook and artwork integration, and `docs/workbook-fixes-2026-09-12.md` for the latest audit fixes and validation. These records distinguish implemented rules from the work the user deferred. `docs/verification.md` and `evidence/build-results.json` retain earlier build history. A feature being present in source is not proof that its tests passed or that it matches a reference image. Failed or unavailable checks are retained. The iPhone simulator build and landscape controls were tested on 10 September 2026. A signed Debug build was installed and its game screen verified on an iPhone 15 Pro Max running iOS 27 on 18 September 2026. Android testing and sustained mobile performance remain unverified.

This project must not be described as release-ready or as a verified complete reproduction while the outstanding items in `docs/feature-matrix.md` remain unresolved.

## Desktop prerequisites

Use a C++20 compiler, CMake 3.24 or newer, Ninja, and Git. Python 3 with `openpyxl` and Pillow is needed only for content import and asset tools, not for the game runtime.

On a Debian or Ubuntu development system, the source dependency build also needs FreeType development files and the development packages for the chosen SDL video and audio drivers. A practical starting set is:

```sh
sudo apt-get install build-essential cmake ninja-build git pkg-config \
    libfreetype6-dev libpng-dev libx11-dev libxext-dev libxrandr-dev \
    libxcursor-dev libxi-dev libxfixes-dev libxss-dev libasound2-dev \
    libegl1-mesa-dev libgl1-mesa-dev
```

On macOS, install the compiler command-line tools, CMake, Ninja, and FreeType. A native macOS build must be made on macOS. A Linux executable is not a macOS application.

The CMake files pin SDL 3.4.12 for iOS and SDL 3.2.20 for other platforms, plus SDL_image 3.2.4, SDL_ttf 3.2.2, and nlohmann/json 3.11.3 when fetching dependencies. iOS requires the newer SDL scene lifecycle to launch when built with the iOS 27 SDK. Compatible locally installed dependencies may satisfy `find_package` first. The exact dependency chosen should be checked in the configuration log.

## Fonts

The app bundles its current display and body fonts with their licenses under `assets/fonts`. To restore the earlier optional font set:

```sh
python3 tools/setup_fonts.py
```

This obtains Luckiest Guy and Baloo 2 plus their license records. The game also supports `AQUARIUM_DISPLAY_FONT` and `AQUARIUM_BODY_FONT` file paths and common local system-font fallbacks. System fallbacks change the typography and must not be mistaken for a reference match. The running application does not download fonts or artwork.

## Build and run

```sh
cmake --preset desktop
cmake --build --preset desktop --parallel 4
```

The executable is named `aquarium` in the desktop build directory selected by `CMakePresets.json`. Run it with the bundled asset directory, for example:

```sh
./build/desktop/aquarium --assets ./assets
```

Use `aquarium --help` to inspect the actual command-line options. `--fresh` opens an ephemeral new-game session. `--save PATH` selects an explicit local save. Do not use a production save when running review fixtures.

## Startup and play

The game restores your save, draws the current reef loading screen and prepares the Shop catalog, tank switcher and Tank Shop before opening the aquarium. Artwork, smaller texture versions and card labels are cached before browsing, so scrolling does not decode images as cards enter view. Build outputs replace their asset folders so deleted artwork cannot remain in a bundle.

Dialogs and popovers open with a short bounce. Their frames, artwork and controls move together, and clicks follow the visible controls during the animation. Reduce motion skips the bounce.

- Open Shop to browse fish, plants, decorations, treasure, backgrounds and tanks. Fish cards select a species for placement. Each completed tap in the water buys and places an egg. Done or Escape cancels placement for free.
- Choose a plant or decoration in Shop, then tap or drag in the tank to position its preview. The green tick buys and saves one copy at its Shop price, then shows the floating currency cost. The active item stays sharp above the lightly blurred tank. The red cross or Escape cancels for free. Both icons sit beside or below the preview.
- Drag Food into the tank to pour a stream of free food. Keep dragging to scatter pellets through the water. Release to stop. You can also choose Food or press F, then drag within the water. Tapping the water does not drop food. Choose the Sell net or press S to sell eligible fish with one tap. Nearby fish gently make room for each other and settle close to where they were, clear of the edges and controls. Fish that already have room stay in place. Eggs and Babies cannot be sold. Opening a menu turns these tools off.
- Choose the four-arrow button above Sell to enter Layout mode. Drag an item and release to save its position. The tick or an outside tap ends editing and keeps the item where you left it. The red remove button returns it to inventory. Exit leaves Layout mode. Bag groups stored plants and decorations into stacks such as ×3. Choose Place to bring one copy into the tank and edit it; the other copies stay stored.
- Tap a fish without a tool to see its care, growth and sale value. Fish stay in the tank until sold. Use the heart to favorite a fish and protect it from selling. Sell pays the current coin and XP value once. Eggs, adults and premium fish share the tank capacity.
- Tap empty water without a tool to make a ripple. Nearby fish briefly speed up along their current course, then return to their usual swimming. Reduced motion uses a fading ring and a gentler response.
- Tank opens the porthole switcher for owned tanks. Buy new tanks at their required levels in Shop's Tanks tab. Owned tanks can expand from 10 to 40 fish in steps of five at any level, using either Coins or Pearls. Tank 6 remains unavailable until its catalog rules are defined.
- Buy or select a background from Shop. Each tank keeps its own background.

Settings opens the turquoise card menu with amber controls. Music, music volume, sound effects, reduced motion and vibration save as you change them. About & support includes version details and a button to copy support information. Music uses the Coral Promenade loop. Vibration uses the iPhone's haptic feedback when available. On iOS, reduced motion follows the device preference until you choose a setting in the game.

Rewards shows progress toward one pearl for every twenty adult coin-fish sales. The pearl is credited automatically, and the reward can be earned repeatedly, including at level 40. Projects currently opens an empty dialog. Treasure purchases are also unavailable. Bag uses the Shop layout inside a dialog, with Plants and Decorations tabs for stored items; its fish inventory remains unavailable. Stored items and settings remain in saves; removing the old screens does not erase that data.

New fish now use the [active-play economy](docs/active-economy.md), with faster XP gains, pearl fish priced from 1 to 14 pearls at launch, and smoother tank prices. Already owned fish retain their saved sale quotes.

The launch script saves to `local-data/save.json`. Running the executable directly without `--save` uses its existing platform save location. Use `--fresh` or a fixture for reviews that must not change a player's save.

```sh
./build/desktop/aquarium --assets ./assets --fixture shop --ui-variant fish
./build/desktop/aquarium --assets ./assets --fixture tanks
./build/desktop/aquarium --assets ./assets --fixture tank-switcher
```

See [the current interface guide](docs/clay-hud.md) for controls and [the feature matrix](docs/feature-matrix.md) for remaining work.

## Tests

The domain test build requires no SDL, window, GPU, or mobile SDK:

```sh
cmake --preset domain
cmake --build --preset domain --parallel 4
ctest --preset domain --output-on-failure
```

A separate sanitizer preset applies AddressSanitizer and UndefinedBehaviorSanitizer to project code:

```sh
cmake --preset sanitize
cmake --build --preset sanitize --parallel 4
ctest --preset sanitize --output-on-failure
```

Run the full evidence-producing workflow with:

```sh
python3 tools/build_verify.py
```

It retains each command, exit code, elapsed time, and log. Screenshots or clips are produced only when a native executable can actually run. The script's nonzero exit status means at least one check failed or was unavailable.

## Content and documentation

`design/aquarium_game_design_v4.xlsx` is the active balancing source workbook. `assets/content.json` is generated runtime content. `assets/workbook.json` retains source cells and formulas for inspection. Import tools are in `tools/`; inspect each tool's `--help` before changing a content-generation command.

Additional documentation:

- `docs/architecture.md`: ownership and module boundaries.
- `docs/save-format.md`: persisted state, validation, and recovery.
- `docs/tuning.md`: visual, motion, and input constants.
- `docs/assumptions.md`: source conflicts and unspecified details.
- `docs/feature-matrix.md`: implementation and verification status.
- `docs/workbook-fixes-2026-09-12.md`: fixes from the detailed workbook audit and remaining undefined rules.
- `docs/mobile.md`: Android and iOS build configuration.
- `docs/verification.md`: actual evidence and limitations.

No online social service, payment provider, cloud save, or store purchase is connected. The local NPC path does not pretend to be remote multiplayer.
