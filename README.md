# Fishius: a native C++20 and SDL3 aquarium game

The game uses native SDL rendering, textured fish meshes, an event-driven care model, physical food, a local wallet, tanks, growing and display fish, inventory, and JSON saves. The supplied behavior contract is the movement and interaction reference. The supplied spreadsheet is the catalog source. No original game source or running application is required.

Open [Run Aquarium Studio.command](Run%20Aquarium%20Studio.command) to edit the
general dialog, currency shop cards, tank upgrade panel and currency HUD.
The editor shares the game renderer and supports reusable components, layout
rules, direct text editing, comparisons, purchase tests and recoverable drafts.
See the [Studio guide](docs/aquarium-studio.md) for controls and verification.

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

## Startup

Fishius shows a reef loading screen while it prepares fish artwork, all Shop and Collection pages, menu labels, tools, and the starting aquarium. Progress follows completed preparation steps. The Shop keeps prepared labels in a separate, bounded cache, so swipes do not trigger first-use image decoding or text rendering. Cards outside the visible area are skipped.

New development games use a separate `save-v4.json` file. Older saves are not migrated. Use `--loading-capture PATH` to capture the loading screen during a review run.

## Play

Open Shop and choose a fish to start egg placement. The egg follows the pointer, and each primary tap buys and places one egg at that spot. The hint shows the species and price per egg. Keep tapping to place more, or choose Done or press Escape to stop. Choosing a fish or cancelling placement costs nothing. A failed purchase ends placement without charging. Free Bubble Eye Goldfish and premium fish create permanent companions and stop after one placement.

Food drops pellets. Opening another menu turns off Food or Sell and returns to normal selection. Closing the menu keeps those tools off. Hungry fish notice and pursue them at individual times. Tap a fish to open its details. Use Arrange tank from Bag to enter decorate mode, then tap a plant or decoration to select it. Drag it to preview a move, then press the green tick to save the position. The red X cancels the move. Stash stores the selected copy. Growing fish and eggs cannot be stored. Kept adults and premium companions can be stored and restored through Bag. Bag also opens stored decor so you can place it in the active tank without buying it again. Selecting a plant or decoration in Shop opens a draggable preview. Only the green tick purchases and places it. The red X cancels for free. Stored items use the same controls. The net opens the compact fish popover. Rehome shows the exact current coins and XP, with a small inline confirmation. At adulthood, Keep gives the same reward and retains a nonproducing display fish.

Tank controls switch owned tanks, unlock the next tank, and buy the next capacity step using either the workbook coin price or the pearl price. Each price button charges only its own currency. Growing capacity is 10, 15 or 20 per tank. Display fish have eight separate slots per tank. Keep stores excess display fish in Bag. Feeding is free, and hunger only pauses growth. Eggs still hatch after six seconds. Decor uses its own placement limit.

Keyboard shortcuts: F for Food, S for Sell, B for Shop, I for inventory, and Escape to close or cancel. Mouse and touch use the same ownership rules.

Each level-up pays its exact one-time v4 grant. Buying fish or decor gives no account XP. Keep and Rehome settle a saved purchase quote once, even after a retry or reload. See `docs/workbook-implementation.md` for the completed steps and deferred systems.

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
