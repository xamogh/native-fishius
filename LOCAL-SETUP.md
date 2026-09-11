# Run Aquarium on this Mac

Double-click `Run Aquarium.command` to open the game. The launcher stores your progress in `local-data/save.json`.

The project remote is `https://github.com/xamogh/native-fishius`. The existing baseline was pushed to `main`; later workbook implementation changes remain local until a new push is authorized.

## Build trees moved on 10 September 2026

Both build trees used to carry absolute paths to the folder this project was copied from, so CMake refused to reconfigure them here and Ninja reported `manifest 'build.ninja' still dirty after 100 tries`. `build/desktop` and `build/ios-simulator` were reconfigured from this folder and now stand on their own. Nothing outside this folder is needed to build, apart from the tool paths below.

The iOS target used to bundle its assets through a configure-time glob, which silently missed the artwork in `assets/skin` because that folder was generated after the build tree was configured. It now copies the whole `assets` tree after each link, so regenerated assets and file types outside the old pattern list (the `.otf` fonts) always reach the bundle.

## Historical setup completed on 9 September 2026

- Built the native Apple Silicon executable with Apple Clang 21.
- Used the installed SDL 3.4.12 and FreeType 2.14.3 libraries.
- Downloaded SDL_image 3.2.4, SDL_ttf 3.2.2, and nlohmann/json 3.11.3 through CMake.
- Imported all 46 fish from the supplied workbook. The importer now recognizes `Unlock Lvl` and selects the numeric `Stage Time (hrs)` column.
- Generated the missing artwork using the supplied asset script and downloaded the intended fonts with their license records.
- Validated the 138 placeholder fish images used at that time. These are superseded by the canonical species art below.
- Ran a 30-frame windowed smoke check using the Metal renderer. See `evidence/macos-smoke.png` and `evidence/macos-smoke.json`.
- Ran the existing domain tests: 23 of 24 passed. The failing test is `one pellet feeds one fish with stable tie order`. Its fixture sets a notice delay before calling `Domain::install`, which clears that delay, so the test's immediate feeding expectation is not met. The test and runtime behavior were left unchanged.

The `evidence` folder also contains reports from the original archive. Those older reports describe the original build attempt, not this Mac setup.

## Build again

From this project folder:

```sh
/Users/amoghrijal/Documents/Codex/2026-09-09/plea/work/build-tools/bin/cmake --build --preset desktop --parallel 4
/Users/amoghrijal/Documents/Codex/2026-09-09/plea/work/build-tools/bin/ctest --preset desktop --output-on-failure
```

Build tools and Python asset libraries are installed in the task's `work/build-tools` environment. This build uses libraries on this Mac and is not a portable application package.

## Run in the iPhone simulator

Double-click `Run iPhone Simulator.command`. It opens the installed iPhone 17 Pro simulator with iOS 26.5 and launches the native iOS app in landscape. Simulator progress is saved separately from the desktop game.

The simulator app was built and launched on 9 September 2026. The game rendered and a tap on Next advanced its tutorial. Physical iPhone installation has not been tested.

The camera cutout overlap was fixed on 10 September 2026. The game now fills the whole display, and the left navigation column runs as two buttons above the cutout and two below it, leaving the middle of that edge free. Settings moved off the column to the menu button in the top-right corner. Tapping in the simulator was not scripted, so the new hit areas have not been confirmed by touch.

The navigation buttons scale from a single reference size of 111x119 with a 45 corner radius. Sizing them without that tie let the corner radius reach half the button height, which collapsed the nine-slice's middle row, flattened the pill into an ellipse and pushed the icon and label past its edges. Labels are also clamped to the width the pill actually has at their own baseline, since the pill has narrowed into its corner radius by that height.

The iOS build includes the font files and their license records. It uses SDL 3.2.20, SDL_image 3.2.4 with its portable image loader, SDL_ttf 3.2.2, and FreeType 2.13.2 built for the simulator. Build logs and dependency sources are in `build/ios-deps`; startup logs are in `evidence/ios-stdout.log` and `evidence/ios-stderr.log`.

To rebuild the configured simulator target:

```sh
/Users/amoghrijal/Documents/Codex/2026-09-09/plea/work/build-tools/bin/cmake --build build/ios-simulator --parallel 6
```

That tree was reconfigured on 10 September 2026 with the FreeType paths given explicitly, because the iOS sysroot stops `find_package` from picking them up on its own, and with `BUILD_SHARED_LIBS=OFF`, which SDL_image needs in order to use the static SDL3 target instead of looking for an installed one. To configure it again from scratch:

```sh
BIN=/Users/amoghrijal/Documents/Codex/2026-09-09/plea/work/build-tools/bin
PATH="$BIN:$PATH" "$BIN/cmake" -S . -B build/ios-simulator -G Ninja \
  -DCMAKE_MAKE_PROGRAM="$BIN/ninja" \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/platform/ios/ios-toolchain.cmake" \
  -DCMAKE_BUILD_TYPE=Release -DAQ_BUILD_APP=ON -DAQ_BUILD_TESTS=OFF \
  -DBUILD_SHARED_LIBS=OFF \
  -DCMAKE_OSX_SYSROOT=iphonesimulator -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
  -DCMAKE_PREFIX_PATH="$PWD/build/ios-deps/install" \
  -DFREETYPE_INCLUDE_DIR_freetype2="$PWD/build/ios-deps/install/include/freetype2" \
  -DFREETYPE_INCLUDE_DIR_ft2build="$PWD/build/ios-deps/install/include/freetype2" \
  -DFREETYPE_LIBRARY_RELEASE="$PWD/build/ios-deps/install/lib/libfreetype.a"
```


## Aquarium screen reference update, 10 September 2026

The aquarium opens directly into the water view. Its right controls and bottom
corner buttons now use the supplied screenshot artwork. Select also opens Move,
Stash, and Bag. Food activates feeding. The four main right-side icons have
even gaps, and Medicine is hidden for now. The tutorial is skipped for this pass.
The Tanks menu opens to the right of its button and keeps the right-side
controls available. Shop sits slightly farther left and lower on iPhone.
Shop opens as a large modal over a dimmed aquarium. Tapping the backdrop
closes Shop and consumes the tap, so menus behind it do not open.

Saved balances and fish remain live. A separate, temporary reference fixture
provides the six fish, 250 coins, zero pearls, and 14 XP shown in the screenshot:

```sh
./build/desktop/aquarium --assets ./assets --fixture aquarium --still --width 804 --height 415
```

Omit `--still` to let the fixture swim. Fixtures do not overwrite ordinary saves.
The iPhone simulator accepts the same fixture flags through `simctl launch`.
Normal launches continue the user's saved game.

The viewport is 1608 by 830 logical units and grows horizontally on wider phones.
Controls stay at their measured size. The cutout side gets extra space, and touch
targets are at least 44 native points. The physical device frame is not part of
the app artwork. Asset changes now trigger relinking and bundling even when no
C++ file changed.

Native interaction tests cover 804 by 415, 852 by 393, and 667 by 375 points:

```sh
./build/desktop/aquarium_view_tests ./assets
```

See `design-qa.md` and `evidence/aquarium-match` for comparisons and test results.
The feeding-order fixture is corrected. The current domain suite passes all 29 cases. See `docs/workbook-implementation.md` for the latest implementation and verification.

## Workbook implementation, 10 September 2026

The normal game now uses the workbook starter roster, all 46 species, exact quest XP, tank coin and Gift Token costs, and Adult mastery milestones. It includes 40 newly generated species illustrations and six suitable existing illustrations, with common art in every screen. The tutorial, anniversary timing and unspecified rewards are deferred.

Use `--fresh` to try the new starter roster without replacing an existing save. Existing saved fish and balances are preserved. The Shop remains a large modal. Backdrop taps close it without opening another menu.

Current evidence is in `evidence/workbook-*` and `evidence/asset-validation.json`. The older setup notes above describe the earlier build, not the current test result.

## Fish selection details

Selecting a fish opens a compact popover pointing to that fish in the aquarium.
It chooses a side with room and stays inside the screen. The aquarium controls
remain visible. Tap another fish to switch details, use an aquarium tool, or
close the popover by tapping empty water or its close button.

The popover keeps the reference's gold frame, large stage title, centered fish
name, food and XP icons, growth bar and circular stage markers. Decorative
bubbles and the separate glowing fish preview are omitted. It shows the live
care deadline, growth percentage and stage, plus the sale rule or current sale
value. Dead fish retain Revive and Remove actions.

For a safe preview that does not write normal progress:

```sh
build/desktop/aquarium --assets assets --fixture details --width 852 --height 393
```

Add `--still --frames 2 --capture-window --capture /tmp/fish-details.png` for
a fixed screenshot at the window's physical display size. Other review fixtures
are `details-fed`, `details-junior`, `details-adult`, `details-sick`,
`details-dead` and `details-egg`.

Build and mobile verification are recorded in `design-qa.md` and
`evidence/fish-popover-reference/final-tests.log`. The tests run through CTest or directly
with `build/desktop/aquarium_details_tests assets`.

## Sharp rendering and native iPad support, 11 September 2026

Double-click `Run iPad Simulator.command` to install the latest built app and
open it on iPad Pro 11-inch (M5). It launches the normal saved game. Rebuild the
iOS target first after changing code, as described above.

The simulator bundle now explicitly declares both device families. This is
required for the Ninja build to run at native iPad resolution instead of an
enlarged compatibility view. The layout fills the full tablet aspect ratio,
anchors the bottom buttons and keeps Shop as a large modal with an eight-card
tablet layout. Font textures and artwork filtering are sized for native pixels.

The renderer targets 60 frames per second and interpolates fish animation between
simulation steps. CTest now includes `rendering_quality` and tablet interaction
cases. See `design-qa.md` and `evidence/render-quality/final-tests.log` for the
current results. Frame interval reports include presentation wait and exclude
the first 60 frames; the older CPU-only fields remain separately labeled.
