---
title: 软光栅渲染器开发记录
date: 2025-04-01 12:00:00
tags: 
- 计算机图形学
- C++
- 渲染引擎
categories: 技术实践
---

# 软光栅渲染器开发阶段性成果

## 项目概述
我们实现了一个基础的软光栅渲染器，具有以下特点：
- 完全从零实现，不依赖图形API
- 仅使用标准库和基础数学运算
- 支持基本的像素绘制和图像输出

## 核心功能实现

### 1. 数学库
```cpp
// 向量模板类
template<typename T>
struct Vector3 {
    T x, y, z;
    // 向量运算...
};

// 4x4矩阵
struct Matrix4x4 {
    float m[4][4];
    // 矩阵运算和变换...
};
```

### 2. 帧缓冲管理
```cpp
class Framebuffer {
    int width, height;
    std::vector<Vector3<float>> pixels;
    
public:
    // 清屏、像素绘制等操作...
    bool saveToTGA(const std::string& filename);
};
```

### 3. TGA图像输出
实现了Truevision TGA格式的图像输出：
- 支持24位RGB格式
- 包含完整的文件头结构
- 像素数据BGR排列

## 项目结构
```
SoftRasterizer/
├── include/
│   ├── math/       # 数学库
│   └── core/       # 核心渲染组件
├── src/
│   ├── io/         # 文件IO
│   └── core/       # 实现代码
└── CMakeLists.txt  # 构建配置
```

## 使用方法
1. 构建项目：
```bash
cmake -S . -B build
cmake --build build --config Release
```

2. 运行程序：
```bash
./build/Release/SoftRasterizer.exe
```

## 后续计划
- 实现OBJ模型加载
- 添加三角形光栅化
- 支持深度缓冲(Z-buffer)
- 实现基础光照模型

[查看完整代码](https://github.com/yourname/SoftRasterizer)
