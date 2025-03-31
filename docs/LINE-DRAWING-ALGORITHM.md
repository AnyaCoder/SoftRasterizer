---
title: 软光栅直线绘制算法实现
date: 2025-04-01 01:15:00  
tags:
- 计算机图形学  
- C++
- 渲染引擎
categories: 技术实践
---

# 直线光栅化基础算法 - Bresenham实现

## 算法简介
Bresenham算法是计算机图形学中最基础的直线光栅化算法，通过整数运算高效确定最佳逼近直线路径的像素点。

### 核心特点
- 完全整数运算，无浮点计算
- 避免乘除法，仅用加减和位运算
- 一次生成一个像素，时间复杂度O(n)

## 实现原理

### 基本思想
算法通过误差项决定下一个像素的选择：
1. 以x为步进方向
2. 计算Δy/Δx的斜率
3. 维护误差项跟踪实际直线与像素中心的距离
4. 根据误差决定是否增加/decrease y

### 关键优化
```cpp
bool steep = abs(y1 - y0) > abs(x1 - x0); // 是否为陡峭线
if (steep) std::swap(x0, y0); // 统一处理为缓变线
if (x0 > x1) std::swap(x0, x1); // 确保从左到右绘制

int dx = x1 - x0;
int dy = abs(y1 - y0);
int err = dx / 2; // 初始误差
```

## 接口实现

添加到Framebuffer类：
```cpp
class Framebuffer {
public:
    // ...
    void drawLine(int x0, int y0, int x1, int y1, const Vector3<float>& color);
};
```

## 测试用例

测试不同方向的直线绘制：
```cpp
// 水平线（红色）
framebuffer.drawLine(100, 100, 700, 100, Vector3<float>(1,0,0));

// 垂直线（蓝色）  
framebuffer.drawLine(400, 100, 400, 500, Vector3<float>(0,0,1));

// 对角线（绿色）
framebuffer.drawLine(100, 150, 700, 500, Vector3<float>(0,1,0));
```

## 效果验证
生成图像应包含：
1. 正确朝向的3D模型线框
2. 所有边线完整连接
3. 无断裂或缺失像素

### 坐标系统说明
模型渲染时进行了坐标转换：
- X坐标：保持原样 (x1 = (v1.x + 1) * width / 2)
- Y坐标：翻转以符合屏幕坐标系 (y1 = height - (v1.y + 1) * height / 2) 
- Z坐标：暂时忽略

## 继续阅读
- [Bresenham原始论文](http://www.cs.unc.edu/~mcmillan/comp136/Lecture6/Lines.html)
- [算法优化技巧](https://www.cs.helsinki.fi/group/goa/viewing/leikkaus/linealg.html)

[返回项目主页](../)
