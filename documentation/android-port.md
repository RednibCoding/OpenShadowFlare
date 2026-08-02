# Android port

The Android build uses NativeActivity, the Android NDK, EGL/OpenGL ES 3 for
the existing LGL surface presenter, and AAudio for 44.1 kHz stereo output.
It supports Android 8.0 (API 26) or newer.

## Build

Android Studio is optional. Install a JDK 17 plus the Android command-line
SDK tools, then install Android API 35, Build Tools 35.0.0, NDK 27.0.12077973,
and CMake 4.1.2.

```bash
sh tools/android/build-apk.sh
```

The installable debug APK is written to:

```text
build/android/debug/OpenShadowFlare-android-debug.apk
```

The game data is not packaged. The app reads it from private app storage, so
copy a legally obtained `ShadowFlare` directory containing `SFlare.Cfg` and
`System/` after installing the APK. With an emulator connected through ADB:

```bash
# Use ADB=adb when platform-tools is already on PATH. On Git Bash with the
# standard Windows SDK location, replace <your-user> with your account name.
ADB=/...

# From the repository root
tar -C tmp -cf - ShadowFlare | "$ADB" exec-in sh -c 'tar -xf - -C /sdcard/Android/data/org.openshadowflare.game/files'
"$ADB" shell run-as org.openshadowflare.game mkdir -p files
"$ADB" shell "cd /sdcard/Android/data/org.openshadowflare.game/files/ShadowFlare && tar -cf - . | run-as org.openshadowflare.game tar -xf - -C files"
```

The first command uses a temporary external staging folder; the second places
the files in the app-owned directory where Android permits native file access.
Repeat both commands after uninstalling the app, since uninstalling clears its
private data.
