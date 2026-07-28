#!/bin/bash

# OpenShadowFlare DLL Build Script
# Cross-compiles Windows DLLs on Linux using MinGW

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build-win32"
GAME_DIR="$SCRIPT_DIR/../tmp/ShadowFlare"

# MinGW cross-compiler (32-bit for original game compatibility)
CXX="i686-w64-mingw32-g++"

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
for arg in "$@"; do
    case $arg in
        --deploy)
            DEPLOY=true
            ;;
        --help)
            echo "Usage: $0 [--deploy]"
            echo "  --deploy  Copy built DLLs to game folder and backup originals"
            exit 0
            ;;
    esac
done

# Check for MinGW
if ! command -v $CXX &> /dev/null; then
    echo "Error: $CXX not found. Install mingw-w64:"
    echo "  sudo apt install mingw-w64"
    exit 1
fi

# Create a clean build directory without touching anything outside the
# repository-owned output folder.
mkdir -p "$BUILD_DIR"
find "$BUILD_DIR" -mindepth 1 -maxdepth 1 -type f -delete

echo "Building Windows DLLs with MinGW..."
echo "========================================"

# Loop through each directory and compile
for dir in "${dirs[@]}"; do
    echo -n "Compiling $dir... "
    
    # Extra libs for specific DLLs
    EXTRA_LIBS=""
    if [ "$dir" = "RKC_DBFCONTROL" ]; then
        EXTRA_LIBS="-lopengl32 -lwinmm -lddraw"
    fi
    if [ "$dir" = "RKC_DSOUND" ]; then
        EXTRA_LIBS="-lwinmm"
    fi
    if [ "$dir" = "RKC_NETWORK" ]; then
        EXTRA_LIBS="-lws2_32"
    fi
    
    "$CXX" -shared -static-libgcc -static-libstdc++ \
        -std=c++17 \
        -o "$BUILD_DIR/$dir.dll" \
        "$SCRIPT_DIR/$dir/src/core.cpp" \
        "$SCRIPT_DIR/$dir/dll.def" \
        -lgdi32 -lcomdlg32 $EXTRA_LIBS \
        2>&1
    echo "OK"
done

echo "========================================"
echo "All DLLs compiled successfully!"
echo "Output: $BUILD_DIR/"

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
