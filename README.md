# SoftRasterizer

一个从零实现的软光栅渲染器，不依赖图形API，仅使用标准库和基础数学运算。

## 功能特性
- 基础数学库（向量、矩阵运算）
- 帧缓冲管理
- TGA图像输出
- 跨平台构建（CMake）

## 快速开始
```bash
cmake -S . -B build
cmake --build build --config Release
./build/bin/SoftRasterizer.exe
```

详细文档请查看[文档说明](./docs/SOFTRASTERIZER-INTRODUCTION.md)
