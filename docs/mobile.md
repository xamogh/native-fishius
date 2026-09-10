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

Use macOS with Xcode and an iOS SDK. Install the intended fonts and obtain the pinned native sources on that machine. Configure an Xcode project:

```sh
cmake -S . -B build/ios -G Xcode \
  -DCMAKE_TOOLCHAIN_FILE=platform/ios/ios-toolchain.cmake \
  -DCMAKE_OSX_SYSROOT=iphoneos -DAQ_BUILD_TESTS=OFF
cmake --build build/ios --config Release
```

The supplied property list declares landscape orientations for iPhone and iPad. Set your development team and signing identity in Xcode for physical-device installation. Simulator builds need an appropriate simulator sysroot and architecture.

The bundle must retain the `assets` resource directory structure. Verify resource paths, font installation, orientation, interruptions, suspend/resume, and safe areas in the actual bundle. A desktop screenshot at an iPhone-like resolution is not evidence that any of these native behaviors work.

## Unverified platform work

No Android or iOS SDK compilation, application signing, device installation, store readiness, native cutout handling, or sustained mobile performance is certified in this delivery. The feature matrix names additional host gaps that require completion and testing.
