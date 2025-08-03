#!/bin/bash

# Define the path to the Dreamcast toolchain file
export KOS_CMAKE_TOOLCHAIN="/opt/toolchains/dc/kos/utils/cmake/dreamcast.toolchain.cmake"

# Define the source directory and build directory
SOURCE_DIR="${PWD}/.."
BUILD_DIR="${PWD}/build"

# Default options
BUILD_JOBS=$(nproc) # Use all available CPU cores by default

# Parse command-line arguments
while [[ "$#" -gt 0 ]]; do
    case $1 in
        clean) 
            echo "Cleaning build directory..."
            cd "$BUILD_DIR"
            make clean
            rm -rf CMakeFiles CMakeCache.txt Makefile
            exit 0
            ;;
        distclean)
            echo "Removing build directory..."
            cd "$BUILD_DIR"
            make uninstall
            rm -rf "$BUILD_DIR"
            exit 0
            ;;
        *) 
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
    shift
done

# Ensure source directory exists
if [ ! -d "$SOURCE_DIR" ]; then
    echo "Error: Source directory $SOURCE_DIR does not exist"
    exit 1
fi

# Create the build directory if it doesn't exist
mkdir -p "$BUILD_DIR"

# Navigate to the build directory
cd "$BUILD_DIR"

# Set SDL2 directory explicitly to where the SDL2Config.cmake is located
export SDL2_DIR="/opt/toolchains/dc/kos/addons/lib/dreamcast/cmake/SDL2"

# Set SDL2 and KOS paths for SDL2_mixer build
export CMAKE_OPTS="$CMAKE_OPTS -DSDL2_INCLUDE_DIR=${SDL2_DIR}/dreamcast/SDL2 -DSDL2_LIBRARY=${SDL2_DIR}/lib/dreamcast/libSDL2.a"

# Set SDL2 main library path
export CMAKE_OPTS="$CMAKE_OPTS -DSDL2_MAIN_LIBRARY=${SDL2_DIR}/lib/dreamcast/libSDL2main.a"

# # Explicitly set SDL2 as static if needed
# export SDL2_SHARED=OFF
# export CMAKE_OPTS="$CMAKE_OPTS -DSDL2_SHARED=${SDL2_SHARED}"

# Override BUILD_SHARED_LIBS to OFF (or ON depending on your needs)
export BUILD_SHARED_LIBS=OFF
export CMAKE_OPTS="$CMAKE_OPTS -DSDL2MIXER_BUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}"

# Correct KOS_PORTS_DIR path
export KOS_PORTS_DIR="${KOS_BASE}/../kos-ports"

# Set KOS include and lib paths for necessary libraries (e.g., OpusFile)
export KOS_LIB_PATH="${KOS_PORTS_DIR}/lib"
export KOS_INCLUDE_PATH="${KOS_PORTS_DIR}/include"

# --- Disable MOD support with libmodplug ---
export CMAKE_OPTS="$CMAKE_OPTS -DSDL2MIXER_MOD=OFF"
export CMAKE_OPTS="$CMAKE_OPTS -DSDL2MIXER_MOD_MODPLUG=OFF"
# (Removed the modplug_LIBRARY and modplug_INCLUDE_PATH options since they are not needed)

# Disable FluidSynth support if you don't need it
export CMAKE_OPTS="$CMAKE_OPTS -DSDL2MIXER_MIDI_FLUIDSYNTH=OFF"

# Disable libxmp dependencies as no compatible libraries were found
export CMAKE_OPTS="$CMAKE_OPTS -DSDL2MIXER_MOD_XMP=OFF"        # Disable libxmp
export CMAKE_OPTS="$CMAKE_OPTS -DSDL2MIXER_MOD_XMP_LITE=OFF"   # Disable libxmp-lite
export CMAKE_OPTS="$CMAKE_OPTS -DSDL2MIXER_MOD_XMP_SHARED=OFF"   # Disable dynamic libxmp loading
export CMAKE_OPTS="$CMAKE_OPTS -DOPUSFILE_HEADER=${KOS_INCLUDE_PATH}/opusfile/opusfile.h" 
export CMAKE_OPTS="$CMAKE_OPTS -DOPUS_INCLUDE_DIR=/opt/toolchains/dc/kos/../kos-ports/include/opus"

# Disable wavpack support due to missing KOS wavpack library
export CMAKE_OPTS="$CMAKE_OPTS -DSDL2MIXER_WAVPACK=OFF"

# Run CMake to configure the project with the selected options
cmake -DCMAKE_TOOLCHAIN_FILE="$KOS_CMAKE_TOOLCHAIN" \
      -G "Unix Makefiles" \
      -D__DREAMCAST__=1 \
      -Wno-dev \
      $CMAKE_OPTS \
      -DCMAKE_INSTALL_PREFIX=${KOS_BASE}/addons \
      -DCMAKE_INSTALL_LIBDIR=lib/dreamcast \
      -DCMAKE_INSTALL_INCLUDEDIR=include/dreamcast \
      -DOpusFile_INCLUDE_PATH=${KOS_INCLUDE_PATH}/opusfile \
      -DOpusFile_LIBRARY=${KOS_LIB_PATH}/libopusfile.a \
      "$SOURCE_DIR"

# Build the project
make -j"$BUILD_JOBS" install

# Print a message indicating the build is complete
echo "Dreamcast build complete!"
