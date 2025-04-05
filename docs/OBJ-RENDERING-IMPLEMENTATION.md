---
title: OBJ模型加载与三角形渲染实现
date: 2025-04-03 14:00:00  
tags:
- 计算机图形学  
- C++
- 模型渲染
categories: 技术实践
---

# OBJ模型加载与三角形渲染实现

## 坐标系确定
本渲染器使用**左手坐标系**，判断依据：

### 静态分析方法
1. 检查顶点变换：
```cpp
// 没有Z轴反转操作，保持原始方向
screen_coords[j] = vec2i((v.x+1)*fb.width/2, (v.y+1)*fb.height/2);
// Z值保持不变，直接用于深度比较
```

2. 检查法线计算：
```cpp
vec3f normal = (v2-v0).cross(v1-v0).normalized();
// 叉乘顺序决定法线方向，与左手系一致
```

3. 检查光照计算：
```cpp
float intensity = normal.dot(lightDir.normalized());
// 当lightDir=(0,0,1)时，朝前的面(intensity>0)会被渲染
```

### 动态验证方法
1. 创建测试三角形：
```cpp
vertices = {{0,1,0}, {-1,-1,0}, {1,-1,0}}; // 朝向+z
```
2. 观察不同光照方向效果：
   - lightDir(0,0,1) 应可见
   - lightDir(0,0,-1) 应不可见

## 实现概述
本次实现了OBJ模型加载和三角形渲染功能，主要包含：
- OBJ文件格式解析
- 三角形面片渲染
- 基础光照计算
- 背面剔除优化

## 核心实现

### 1. OBJ文件加载
```cpp
bool Model::loadFromObj(const std::string& filename) {
    // 解析顶点数据
    if (type == "v") {
        vec3f v;
        iss >> v.x >> v.y >> v.z;
        vertices.push_back(v);
    }
    // 解析面数据
    else if (type == "f") {
        // 处理v/vt/vn等多种格式
        while (iss >> v) {
            face.push_back(v - 1); // OBJ使用1-based索引
            if (iss.peek() == '/') {
                // 处理纹理/法线坐标...
            }
        }
    }
}
```

### 2. 三角形渲染与光照
```cpp
void Model::renderSolid(Framebuffer& fb, const vec3f& color, 
                       const vec3f& lightDir) {
    // 计算面法线
    vec3f normal = calculateFaceNormal(face);
    
    // 光照计算（Lambert模型）
    float intensity = normal.dot(lightDir.normalized());
    if (intensity > 0) { // 背面剔除
        vec3f shadedColor = color * intensity;
        
        // 三角形光栅化
        fb.drawTriangle(x0,y0, x1,y1, x2,y2, shadedColor);
    }
}
```

### 3. 法线计算
```cpp
vec3f Model::calculateFaceNormal() const {
    vec3f edge1 = v1 - v0;
    vec3f edge2 = v2 - v0;
    return edge1.cross(edge2).normalized();
}
```

## 关键技术点

1. **OBJ格式解析**：
   - 支持顶点/纹理/法线坐标
   - 处理多种面定义格式(v, v/vt, v//vn, v/vt/vn)
   - 1-based到0-based索引转换

2. **渲染优化**：
   - 背面剔除：跳过dot product ≤ 0的面
   - 扫描线算法：高效三角形填充
   - 法线插值：使用顶点法线或几何法线

3. **光照模型**：
   - 简单Lambert漫反射
   - 光线方向归一化处理
   - 颜色强度线性缩放

## 使用方法

```cpp
Model model;
model.loadFromObj("model.obj");

// 设置光照方向(指向屏幕里)
vec3f lightDir(0,0,1); 

// 渲染模型(白色)
model.renderSolid(fb, vec3f(1,1,1), lightDir);
```

## 效果验证
渲染测试模型后应得到：
- 正确朝向的面片被渲染
- 背对光源的面片被剔除
- 光照强度随角度变化

## 后续计划
- 实现Z-buffer深度测试
- 添加纹理映射支持
- 实现Phong光照模型
