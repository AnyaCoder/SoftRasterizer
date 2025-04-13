---
title: Blinn-Phong 着色器实现
date: 2025-04-06 13:00:00
tags: 
- 计算机图形学
- C++
- 渲染引擎
- 着色器
categories: 技术分享
---

# Blinn-Phong 着色器实现

## 概述
Blinn-Phong 着色模型是经典 Phong 模型的改进版本，通过引入半角向量(Halfway Vector)优化了高光计算。我们的实现包含完整的顶点和片段着色器处理流程。

## 核心实现

### 1. 顶点着色器
```cpp
Varyings BlinnPhongShader::vertex(const VertexInput& input) {
    Varyings output;
    // 计算世界空间位置
    vec4f worldPos4 = uniform_ModelMatrix * vec4f(input.position, 1.0f);
    output.worldPosition = Vector3<float>(worldPos4.x, worldPos4.y, worldPos4.z);
    
    // 变换法线到世界空间
    vec4f worldNormal4 = uniform_NormalMatrix * vec4f(input.normal, 0.0f);
    output.worldNormal = worldNormal4.xyz().normalized();
    
    // 传递UV坐标
    output.uv = input.uv;
    
    // 计算裁剪空间位置
    output.clipPosition = uniform_MVP * vec4f(input.position, 1.0f);
    return output;
}
```

### 2. 片段着色器
片段着色器实现了完整的 Blinn-Phong 光照模型：
```cpp
bool BlinnPhongShader::fragment(const Varyings& input, Vector3<float>& outColor) {
    // 标准化法线和视线方向
    Vector3<float> N = input.worldNormal.normalized();
    Vector3<float> V = (uniform_CameraPosition - input.worldPosition).normalized();
    
    // 材质属性
    Vector3<float> matDiffuse = uniform_Material.diffuseColor;
    if (uniform_Material.hasDiffuseTexture()) {
        matDiffuse = matDiffuse * uniform_Material.diffuseTexture.sample(input.uv.x, input.uv.y);
    }
    // ...其他材质属性处理
    
    // 光照计算
    Vector3<float> totalColor = uniform_AmbientLight * uniform_Material.ambientColor;
    for (const auto& light : uniform_Lights) {
        // 计算光线方向
        Vector3<float> L = light.getDirectionTo(input.worldPosition);
        
        // 漫反射计算
        float diffFactor = std::max(0.0f, N.dot(L));
        Vector3<float> diffuse = matDiffuse * light.color * diffFactor;
        
        // Blinn-Phong 高光计算
        Vector3<float> H = (L + V).normalized();
        float specFactor = fastPow(std::max(0.0f, N.dot(H)), matShininess);
        Vector3<float> specular = matSpecular * light.color * specFactor;
        
        totalColor += diffuse + specular;
    }
    
    // 颜色钳制
    outColor = totalColor.clamp(0.0f, 1.0f);
    return true;
}
```

## 关键技术点

### 1. 快速幂计算
```cpp
template <typename T>
T fastPow(T base, int n) {
    // 使用快速幂算法优化高光计算
    T res = static_cast<T>(1);
    while (n) {
        if (n & 1) res = res * base;
        base = base * base;
        n >>= 1;
    }
    return res;
}
```

### 2. 光照类型支持
- 方向光(Directional Light)
- 点光源(Point Light)
- 环境光(Ambient Light)

### 3. 材质系统
- 漫反射颜色/贴图
- 高光颜色
- 光泽度(Shininess)
- 环境光反射率

## 使用方法
1. 创建 BlinnPhongShader 实例
2. 设置必要的 uniform 变量:
   - 模型、视图、投影矩阵
   - 材质属性
   - 光源参数
3. 绑定到渲染器使用

## 性能优化
1. 使用快速幂算法优化高光计算
2. 提前终止无效的光照计算
3. 向量运算的规范化处理

## 后续改进计划
- 添加法线贴图支持
- 实现 PBR 材质系统
- 支持多光源阴影计算
