#!/usr/bin/env sh

set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
ANDROID_DIR="$ROOT_DIR/platform/android"
GRADLE_WRAPPER="$ANDROID_DIR/gradlew"
LOCAL_PROPERTIES="$ANDROID_DIR/local.properties"
GRADLE_APK="$ANDROID_DIR/app/build/outputs/apk/debug/app-debug.apk"
OUTPUT_DIR="$ROOT_DIR/build/android/debug"
OUTPUT_APK="$OUTPUT_DIR/OpenShadowFlare-android-debug.apk"

if [ ! -f "$GRADLE_WRAPPER" ]; then
    echo "Android Gradle wrapper not found: $GRADLE_WRAPPER" >&2
    exit 1
fi

if [ ! -f "$LOCAL_PROPERTIES" ]; then
    cat >&2 <<EOF
Android SDK location is missing: $LOCAL_PROPERTIES

Copy platform/android/local.properties.example to local.properties, then replace
its SDK path with yours.

See documentation/android-port.md for Windows and Linux examples.
EOF
    exit 1
fi

sh "$GRADLE_WRAPPER" \
    -p "$ANDROID_DIR" \
    :app:assembleDebug \
    "$@"

if [ ! -f "$GRADLE_APK" ]; then
    echo "Gradle did not produce the expected APK: $GRADLE_APK" >&2
    exit 1
fi

mkdir -p "$OUTPUT_DIR"
cp "$GRADLE_APK" "$OUTPUT_APK"
echo "APK: $OUTPUT_APK"
