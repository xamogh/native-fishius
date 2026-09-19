# Native mobile hosts

These are native SDL hosts for the shared C++ game. They are not browser wrappers. Their existence is separate from successful mobile compilation or device testing.

## Android

Prerequisites are Java 17, Gradle 8.9, Android SDK 35, NDK 27.2.12479018, and the CMake version selected in the Android project. The Android Gradle plugin is pinned to 8.7.3. The build currently targets arm64-v8a and x86_64.

Obtain SDL's Java host and pinned native dependency sources:

```sh
python3 tools/bootstrap_mobile.py
python3 tools/setup_fonts.py
```

Open `platform/android` in Android Studio or run:

```sh
cd platform/android
gradle wrapper --gradle-version 8.9
gradle assembleDebug
```

No pre-generated Gradle wrapper JAR is required by the source archive. The command above creates it on the developer's machine.

`AquariumActivity` subclasses SDLActivity, installs bundled artwork in private application storage, and passes explicit asset and save paths to native main. The manifest selects sensor landscape. SDL handles lifecycle and input dispatch; the native session cancels gestures and saves on backgrounding.

The native dependency configuration needs to be checked against the locally bootstrapped sources, including FreeType and PNG dependencies. No successful Android NDK compilation is asserted by this document. Missing SDKs, unresolved satellite dependency cross-compilation, or Java-host integration errors must be reported as build failures, not described as a working APK.

Release signing requires the application owner's key. No signing key, store identity, billing connection, or Play publication is included.

## iOS

Use macOS with Xcode and an iOS SDK. See [LOCAL-SETUP.md](../LOCAL-SETUP.md) for this Mac's build tools, dependencies, and simulator instructions.

iOS builds require SDL 3.4.12 or newer. SDL 3.2.20 uses the older app lifecycle, which causes an immediate launch crash on iOS 27 when built with the iOS 27 SDK. SDL 3.4 provides the scene lifecycle required by [Apple's migration guide](https://developer.apple.com/documentation/uikit/transitioning-to-the-uikit-scene-based-life-cycle). CMake rejects older SDL source overrides. If an existing build sets `FETCHCONTENT_SOURCE_DIR_SDL3`, update it to the newer source directory or remove that cache entry with `cmake -U FETCHCONTENT_SOURCE_DIR_SDL3 -S . -B BUILD_DIRECTORY` to fetch the pinned release.

### Run on your iPhone

The existing `build/ios-device` directory is configured for physical iPhones and iPads: arm64, the `iphoneos` SDK, and iOS 15 or later. It uses the local iOS libraries in `build/ios-device-deps`. Keep this build separate from `build/ios-simulator`.

1. Connect and unlock your iPhone. Accept **Trust This Computer** if asked.
2. Double-click **`Open iPhone Project.command`** to refresh and open the current project. Choose the **aquarium** scheme and your iPhone as the run destination. The current project is `build/ios-device/FishiusAquarium.xcodeproj`. Close the older `FishXAquarium.xcodeproj` window; it omits newer source files and can fail with undefined-symbol linker errors.
3. In Xcode, sign in under **Settings > Apple Accounts** if needed. Select the **aquarium** target, open **Signing & Capabilities**, enable **Automatically manage signing**, and choose your team. Keep the existing bundle identifier unless Xcode reports that your team cannot use it. Follow [Apple's device signing instructions](https://developer.apple.com/documentation/xcode/running-your-app-on-simulated-or-physical-devices).
4. On the phone, enable **Settings > Privacy & Security > Developer Mode**, restart, and confirm. This setting appears after pairing the phone with Xcode. See [Apple's Developer Mode instructions](https://developer.apple.com/documentation/xcode/enabling-developer-mode-on-a-device).
5. Click **Run** or press **Command-R**. After the app opens, check that the Coin and Pearl **+** buttons open **Currency Shop** directly on the matching tab. Check the dialog by attempting a tank purchase without enough Coins or Pearls. Confirm that it lists only the currency selected for the purchase, even when both balances are low. Check that each price button can buy a tank or upgrade when the other currency balance is zero.

A free Personal Team can install the app on your own phone. Its provisioning profile expires after seven days, so you must rebuild and reinstall after expiry. See [Apple's account guide](https://developer.apple.com/help/account/basics/about-your-developer-account).

### Build for the iPhone simulator

For the iPhone 17 simulator, use **`Open iPhone Simulator Project.command`**
instead. It opens the separate Xcode project in `build/ios-simulator-xcode`,
which links the simulator FreeType library from `build/ios-deps/install`.
Select **aquarium** and **iPhone 17**, then press **Command-R**. Selecting a
simulator in `build/ios-device` fails at linking because that project uses
FreeType compiled for a physical phone.

### Preserve signing when CMake regenerates the project

Xcode edits to a generated project can be replaced by CMake. Store the team and automatic signing in the local CMake cache. For this existing device build, run the following from the repository root, using the CMake executable listed in `LOCAL-SETUP.md` and replacing `YOUR_TEAM_ID` with your team ID:

```sh
cmake -S . -B build/ios-device \
  -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM=YOUR_TEAM_ID \
  -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGN_STYLE=Automatic
```

This reuses the existing device dependency configuration. Repeat it when changing teams. It does not install the app; use Xcode's **Run** action for installation and launch.

The supplied property list declares landscape orientations for iPhone and iPad. The bundle must retain the `assets` directory structure. On the actual phone, check artwork, fonts, orientation, screen edges, interruptions, and saved progress after backgrounding and reopening the app.

## Verification limits

A signed Debug build was installed and launched on the iPhone 15 Pro Max running iOS 27 on 18 September 2026 after updating SDL and restoring the missing bundled body font. A device screenshot confirmed that the Shop screen rendered, and the app remained running. The desktop startup smoke test passed. Full visual and touch checks, interruptions, and sustained performance on the phone remain unverified. Earlier iOS simulator runs are recorded in `LOCAL-SETUP.md`; simulator runtime was not retested for this fix. Android testing and store readiness remain unverified. The feature matrix lists additional host work.
