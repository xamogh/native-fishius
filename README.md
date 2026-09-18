# Fishius: a native C++20 and SDL3 aquarium game

The game uses native SDL rendering, textured fish meshes, an event-driven care model, physical food, a local wallet, tanks, growing and display fish, inventory, and JSON saves. The supplied behavior contract is the movement and interaction reference. The supplied spreadsheet is the catalog source. No original game source or running application is required.

Open `Run Aquarium.command` to build and run the current interface with your saved game. The old interface and its Studio editor have been removed. `--hud-layout` remains a harmless alias for older launch commands; it no longer selects a different interface.

## Delivery status

Read `docs/workbook-implementation.md` for the workbook and artwork integration, and `docs/workbook-fixes-2026-09-12.md` for the latest audit fixes and validation. These records distinguish implemented rules from the work the user deferred. `docs/verification.md` and `evidence/build-results.json` retain earlier build history. A feature being present in source is not proof that its tests passed or that it matches a reference image. Failed or unavailable checks are retained. The iPhone simulator build and landscape controls were tested on 10 September 2026. Physical device installation, Android testing, signing and sustained mobile performance are not verified here.

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

The CMake files pin SDL 3.2.20, SDL_image 3.2.4, SDL_ttf 3.2.2, and nlohmann/json 3.11.3 when fetching dependencies. Compatible locally installed dependencies may satisfy `find_package` first. The exact dependency chosen should be checked in the configuration log.

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

The game restores your save, draws the current reef loading screen and prepares the opening Shop page, tank switcher and Tank Shop before opening the aquarium. Their artwork and labels are cached so the first click does not decode and upload menu artwork. Other catalog pages load as you browse. Build outputs replace their asset folders so deleted artwork cannot remain in a bundle.

- Open Shop to browse fish, plants, decorations, treasure, backgrounds and tanks. Fish cards select a species for placement. Each completed tap in the water buys and places an egg. Done or Escape cancels placement for free.
- Choose Food or press F, then tap the water to feed. Choose the Rehome net or press S to sell eligible fish with one tap. Eggs and Babies cannot be sold. Opening a menu turns these tools off.
- Tap a fish without a tool to see its care, growth and rewards. Keep retains an adult as a display fish and pays its one-time reward. Rehome from fish details asks for confirmation.
- Tank opens the porthole switcher for owned tanks. Unlock and expand tanks from Shop's Tanks tab, using either Coins or Pearls. Tank 6 remains unavailable until its catalog rules are defined.
- Buy or select a background from Shop. Each tank keeps its own background.

Bag, Projects, Rewards and Settings currently open empty dialogs. Plant and decoration cards show catalog prices, but their purchase and placement flows are not connected to the current interface. Treasure purchases are also unavailable. Stored items and settings remain in saves; removing the old screens does not erase that data.

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
