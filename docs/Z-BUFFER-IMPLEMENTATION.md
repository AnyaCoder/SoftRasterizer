---
title: Z-Buffer Implementation
date: 2025-04-04
categories:
  - Computer Graphics
tags:
  - Rendering
  - Depth Buffer
---

## 概述
本文档详细记录了在SoftRasterizer项目中实现Z-Buffer深度测试的全过程。

## 核心修改

### 1. 帧缓冲类改造
```cpp
// 添加深度缓冲区
std::vector<float> zBuffer;

// 初始化
Framebuffer::Framebuffer(int w, int h) : 
    width(w), height(h),
    pixels(w * h),
    zBuffer(w * h, std::numeric_limits<float>::lowest()) {}

// 清空深度缓冲
void clearZBuffer() {
    std::fill(zBuffer.begin(), zBuffer.end(), 
             std::numeric_limits<float>::lowest());
}
```

### 2. 深度测试实现
```cpp
void setPixel(int x, int y, const vec3f& color, float depth) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        int index = y * width + x;
        // 右手坐标系：z值越大表示越远
        if (depth > zBuffer[index]) {  
            zBuffer[index] = depth;
            pixels[index] = color;
        }
    }
}
```

### 3. 三角形渲染优化
```cpp
void drawTriangle(/* 参数 */) {
    // 顶点排序和退化检测...
    
    // 顶部渲染
    for (int y = y0; y <= y1; y++) {
        // 边界检查
        if (y < 0 || y >= height) continue;
        
        // 坐标插值
        int xa = interpolate(x0, y0, x2, y2, y);
        int xb = interpolate(x0, y0, x1, y1, y);
        float za = (y2 != y0) ? z0 + (z2 - z0) * (y - y0) / (y2 - y0) : z0;
        // ...其余代码
    }
    // 底部渲染类似...
}
```

### 4. 主循环集成
```cpp
// 每帧清空
framebuffer.clear(vec3f(0.1f, 0.1f, 0.1f));
framebuffer.clearZBuffer();
```

## 技术要点
1. **坐标系**：采用右手坐标系，+Z指向观察者后方
2. **深度比较**：使用`>`运算符进行深度测试
3. **初始值**：使用`lowest()`而非`min()`
4. **边界处理**：完善的越界检查和除零保护

## 验证方法
1. 近处物体正确遮挡远处物体
2. 无三角形破碎现象
3. 表面深度过渡平滑
4. 无闪烁或Z-fighting现象
