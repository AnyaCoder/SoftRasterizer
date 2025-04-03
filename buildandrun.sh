#!/bin/bash

rm -rf build
mkdir -p build
cmake -S . -B build
cmake --build build --config Release
cd ./build/bin
./SoftRasterizer