#!/bin/bash

mkdir -p thirdparty

if [ ! -d "thirdparty/SDL2" ]; then
    SDL_URL="https://github.com/libsdl-org/SDL/releases/download/release-2.32.4/SDL2-devel-2.32.4-VC.zip"
    SDL_ZIP="thirdparty/SDL2-devel-2.32.4-VC.zip"
    curl -L -o "$SDL_ZIP" "$SDL_URL"
    unzip -o "$SDL_ZIP" -d thirdparty
    mv thirdparty/SDL2-2.32.4 thirdparty/SDL2
fi

if [ ! -d "thirdparty/yaml-cpp" ]; then
    YAML_URL="https://github.com/jbeder/yaml-cpp/archive/refs/tags/0.8.0.zip"
    YAML_ZIP="thirdparty/yaml-cpp-0.8.0.zip"

    curl -L -o "$YAML_ZIP" "$YAML_URL"
    unzip -o "$YAML_ZIP" -d thirdparty
    mv thirdparty/yaml-cpp-0.8.0 thirdparty/yaml-cpp
fi


# Optional clean
if [ "$1" == "clean" ]; then
    rm -rf build
    echo "Build directory cleaned"
fi

# Build
mkdir -p build
cmake -S . -B build
cmake --build build --config Release -j$(nproc)
cd ./build/bin
./SoftRasterizer