# Android port

OpenShadowFlare runs on Android 8.0 (API 26) or newer as a NativeActivity. It
draws through the TWL backend (EGL + OpenGL ES 2, GPU-scaled to an aspect-fit
rectangle) and plays audio through TAL on AAudio (44.1 kHz stereo). The game is
the native shared library `libmain.so`; there is no Java or Kotlin code.

## What to install

Android Studio is optional. Install a JDK 17 and the Android command-line SDK
tools, then use them to install:

- Android SDK Platform 35
- Build Tools 35.0.0
- NDK 27.0.12077973
- CMake 4.1.2

### Set the SDK location

Gradle reads the SDK path from the untracked
`platform/android/local.properties`. Copy the tracked template and edit the path
for your machine:

```bash
cp platform/android/local.properties.example platform/android/local.properties
```

Windows (forward slashes are fine):

```properties
sdk.dir=C:/Users/<your-user>/AppData/Local/Android/Sdk
```

Linux:

```properties
sdk.dir=/home/<your-user>/Android/Sdk
```

Do not commit this file; it is machine-specific and already in `.gitignore`.

## Build

```bash
sh tools/android/build-apk.sh
```

The installable debug APK is written to:

```text
build/android/debug/OpenShadowFlare-android-debug.apk
```

## Game data

The retail data is not packaged. After installing the APK, copy `ShadowFlare` directory to the app's external files directory so the game finds it at:

```text
/sdcard/Android/data/org.openshadowflare.game/files/ShadowFlare/
```

That folder must hold the retail data at its top level, for example:

```text
ShadowFlare/
├── SFlare.Cfg
├── System/
├── Map/
├── Player/
├── Character/
├── Scenario/
└── Save/
```
