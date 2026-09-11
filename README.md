# Aquarium: a native C++20 and SDL3 game project

The game uses native SDL rendering, textured fish meshes, an event-driven care model, physical food, a local wallet, tanks, inventory, quests, and JSON saves. The supplied behavior contract is the movement and interaction reference. The supplied spreadsheet is the catalog source. No original game source or running application is required.

## Delivery status

Read `docs/workbook-implementation.md` for the current workbook and artwork update. It documents the requested deferrals and the latest evidence. `docs/verification.md` and `evidence/build-results.json` retain earlier build history. A feature being present in source is not proof that its tests passed or that it matches a reference image. Failed or unavailable checks are retained. The iPhone simulator build and landscape controls were tested on 10 September 2026. Physical device installation, Android testing, signing and sustained mobile performance are not verified here.

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

## Play

Open Shop, select a fish, and place eggs in the aquarium. Selecting Buy only arms the placement tool. Each subsequent valid placement performs its own transaction. Done or Escape cancels placement.

Food drops pellets. Hungry fish notice and pursue them at individual times. Select opens fish details. Move drags a fish. Stash moves a living fish or egg into paused inventory. Bag restores it into the active tank without buying it again. Sell aims a net at Junior or older living fish.

Tank controls switch owned tanks, unlock the next tank, and buy the next capacity step using the workbook coin and Gift Token costs. Revival costs one pearl and can temporarily exceed nominal capacity, as required by the behavior contract. Purchases and inventory restoration are blocked while full.

Keyboard shortcuts: F for Food, S for Sell, B for Shop, I for inventory, M for Move, and Escape to close or cancel. Mouse and touch use the same ownership rules.

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

`design/aquarium_game_master_model.xlsx` is the supplied workbook, unchanged. `assets/content.json` is generated runtime content. `assets/workbook.json` retains source cells and formulas for inspection. Import tools are in `tools/`; inspect each tool's `--help` before changing a content-generation command.

Additional documentation:

- `docs/architecture.md`: ownership and module boundaries.
- `docs/save-format.md`: persisted state, validation, and recovery.
- `docs/tuning.md`: visual, motion, and input constants.
- `docs/assumptions.md`: source conflicts and unspecified details.
- `docs/feature-matrix.md`: implementation and verification status.
- `docs/mobile.md`: Android and iOS build configuration.
- `docs/verification.md`: actual evidence and limitations.

No online social service, payment provider, cloud save, or store purchase is connected. The local NPC path does not pretend to be remote multiplayer.
