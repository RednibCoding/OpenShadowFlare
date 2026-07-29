#!/bin/bash

# OpenShadowFlare DLL Build Script
# Cross-compiles Windows DLLs on Linux using MinGW

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
SOURCE_DIR="$SCRIPT_DIR/reconstructed"
GAME_DIR="$PROJECT_ROOT/tmp/ShadowFlare"

# MinGW cross-compiler (32-bit for original game compatibility)
CXX="i686-w64-mingw32-g++"
CC="i686-w64-mingw32-gcc"
AR="i686-w64-mingw32-ar"

# Directories to compile
declare -a dirs=("RK_FUNCTION" 
                 "RKC_DBFCONTROL" 
                 "RKC_DIB" 
                 "RKC_DSOUND" 
                 "RKC_FILE" 
                 "RKC_FONTMAKER" 
                 "RKC_MEMORY" 
                 "RKC_NETWORK" 
                 "RKC_RPG_AICONTROL" 
                 "RKC_RPG_SCRIPT" 
                 "RKC_RPG_TABLE" 
                 "RKC_RPGSCRN" 
                 "RKC_UPDIB" 
                 "RKC_WINDOW")

# Parse arguments
DEPLOY=false
CONFIG=release
while [ "$#" -gt 0 ]; do
    case "$1" in
        --deploy)
            DEPLOY=true
            ;;
        --config)
            if [ "$#" -lt 2 ]; then
                echo "Error: --config needs debug or release." >&2
                exit 1
            fi
            CONFIG="$2"
            shift
            ;;
        --debug)
            CONFIG=debug
            ;;
        --release)
            CONFIG=release
            ;;
        --help)
            echo "Usage: $0 [--config debug|release] [--deploy]"
            echo "  --config  Select the output configuration (default: release)"
            echo "  --debug   Shortcut for --config debug"
            echo "  --release Shortcut for --config release"
            echo "  --deploy  Copy built DLLs to game folder and backup originals"
            exit 0
            ;;
        *)
            echo "Error: unknown argument: $1" >&2
            exit 1
            ;;
    esac
    shift
done

case "$CONFIG" in
    debug)
        OPT_FLAGS=(-O0 -g)
        ;;
    release)
        OPT_FLAGS=(-O2)
        ;;
    *)
        echo "Error: configuration must be debug or release." >&2
        exit 1
        ;;
esac

BUILD_DIR="$PROJECT_ROOT/build/dlls/$CONFIG"

# Check for MinGW
if ! command -v "$CXX" &> /dev/null; then
    echo "Error: $CXX not found. Install mingw-w64:"
    echo "  sudo apt install mingw-w64"
    exit 1
fi
if ! command -v "$CC" &> /dev/null || ! command -v "$AR" &> /dev/null; then
    echo "Error: the 32-bit MinGW C compiler and archiver are required." >&2
    exit 1
fi

# Create a clean build directory without touching anything outside the
# repository-owned output folder.
mkdir -p "$BUILD_DIR"
find "$BUILD_DIR" -mindepth 1 -maxdepth 1 -type f -delete
OBJECT_DIR="$BUILD_DIR/.objects"
mkdir -p "$OBJECT_DIR"
find "$OBJECT_DIR" -mindepth 1 -maxdepth 1 -type f -delete

# LAL is C99 and is linked into RKC_DSOUND. Compile it as C rather than
# relying on the C++ compiler's treatment of .c files.
"$CC" -std=c99 "${OPT_FLAGS[@]}" -c \
    "$SCRIPT_DIR/../thirdparty/lal/lal.c" \
    -o "$OBJECT_DIR/lal.o"
"$CC" -std=c99 "${OPT_FLAGS[@]}" -c \
    "$SCRIPT_DIR/../thirdparty/lal/lal_waveout.c" \
    -o "$OBJECT_DIR/lal_waveout.o"
"$AR" rcs "$OBJECT_DIR/liblal.a" \
    "$OBJECT_DIR/lal.o" \
    "$OBJECT_DIR/lal_waveout.o"

echo "Building Windows DLLs with MinGW ($CONFIG)..."
echo "========================================"

# Loop through each directory and compile
for dir in "${dirs[@]}"; do
    echo -n "Compiling $dir... "
    
    # Extra libs for specific DLLs
    EXTRA_LIBS=()
    EXTRA_OBJECTS=()
    if [ "$dir" = "RKC_DBFCONTROL" ]; then
        EXTRA_LIBS=(-lopengl32 -lwinmm -lddraw)
    fi
    if [ "$dir" = "RKC_DSOUND" ]; then
        EXTRA_OBJECTS=("$OBJECT_DIR/liblal.a")
        EXTRA_LIBS=(-lwinmm)
    fi
    if [ "$dir" = "RKC_NETWORK" ]; then
        EXTRA_LIBS=(-lws2_32)
    fi
    
    "$CXX" -shared -static-libgcc -static-libstdc++ \
        -std=c++17 \
        "${OPT_FLAGS[@]}" \
        -o "$BUILD_DIR/$dir.dll" \
        "$SOURCE_DIR/$dir/src/core.cpp" \
        "$SOURCE_DIR/$dir/dll.def" \
        "${EXTRA_OBJECTS[@]}" \
        -lgdi32 -lcomdlg32 "${EXTRA_LIBS[@]}" \
        2>&1
    echo "OK"
done

echo "========================================"
echo "All DLLs compiled successfully!"
echo "Output: $BUILD_DIR/"
find "$OBJECT_DIR" -mindepth 1 -maxdepth 1 -type f -delete
rmdir "$OBJECT_DIR"

# Deploy if requested
if [ "$DEPLOY" = true ]; then
    echo ""
    echo "Deploying to game folder..."
    
    if [ ! -d "$GAME_DIR" ]; then
        echo "Error: game folder not found at $GAME_DIR" >&2
        exit 1
    fi

    # Move each pristine original aside once. Existing o_* backups are never
    # replaced, so repeated deployments remain recoverable.
    for dir in "${dirs[@]}"; do
        ORIG="$GAME_DIR/$dir.dll"
        BACKUP="$GAME_DIR/o_$dir.dll"
        
        if [ -f "$ORIG" ] && [ ! -f "$BACKUP" ]; then
            echo "  Backing up $dir.dll -> o_$dir.dll"
            mv "$ORIG" "$BACKUP"
        fi
    done
    
    # Copy new DLLs
    for dir in "${dirs[@]}"; do
        echo "  Copying $dir.dll"
        cp "$BUILD_DIR/$dir.dll" "$GAME_DIR/"
    done
    
    echo ""
    echo "Deployment complete! Test with:"
    echo "  cd $GAME_DIR && wine ShadowFlare.exe"
fi
