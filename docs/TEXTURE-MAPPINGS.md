---
title: 增强真实感：为软渲染器添加 AO、高光和光泽度贴图
date: 2025-04-13 15:20:00
tags:
  - 图形学
  - 渲染
  - PBR 基础
  - AO
  - 贴图
  - C++
  - 软渲染器
categories:
  - 技术分享
---

在上一篇文章中，我们成功地为 C++ 软渲染器添加了法线贴图支持，让低模也能展现丰富的表面几何细节。然而，要进一步提升渲染的真实感，我们还需要引入更多控制光照和材质表现的细节。本文将介绍如何继续扩展我们的渲染管线，加入**环境光遮蔽 (Ambient Occlusion - AO)**、**高光颜色 (Specular Color)** 和 **光泽度 (Glossiness)** 贴图。

## 1. 回顾：基础光照与法线贴图

目前，我们的 Blinn-Phong 着色器已经能够处理：

* **漫反射贴图 (Diffuse Map):** 定义物体表面的基础颜色。
* **法线贴图 (Normal Map):** 提供逐像素的法线信息，模拟几何细节。
* **统一的材质属性:** 如环境光颜色 (`ambientColor`)、漫反射颜色 (`diffuseColor`)、高光颜色 (`specularColor`) 和光泽度指数 (`shininess`)，这些属性对整个物体生效。

虽然效果已经不错，但真实世界的材质表现远比这复杂。例如，金属和绝缘体的反光方式不同；物体缝隙中的环境光会更少；表面的粗糙度也会影响高光的形状。

## 2. 新成员：增强细节的纹理贴图

为了更精细地控制渲染效果，我们引入以下三种新的纹理贴图：

### 2.1 环境光遮蔽 (Ambient Occlusion - AO) 贴图

* **作用:** AO 贴图描述了模型表面某一点接收**间接环境光**的程度。它模拟了几何体自身或邻近几何体对环境光的遮挡效果。通常，缝隙、角落、褶皱等难以被环境光照射到的地方，其 AO 值较低（偏黑），而暴露在外的表面 AO 值较高（偏白）。
* **实现方式:** AO 贴图通常是一张**灰度图**。在片元着色器中，我们采样 AO 图得到一个遮蔽因子 `aoFactor` (范围 0.0 到 1.0)。这个因子用于**调制 (乘以)** 最终的环境光贡献。
    ```
    // 伪代码
    AmbientTerm = GlobalAmbientLight * MaterialAmbientColor * aoFactor;
    ```
* **视觉效果:** AO 贴图可以显著增强模型的体积感和细节，尤其是在缺少复杂全局光照计算的简单渲染管线中，能有效地模拟出接触阴影和几何体之间的遮挡感。

{% asset_img ao_map_effect.png AO Map Effect %}
*（示意图：左侧无 AO，右侧有 AO）*

### 2.2 高光颜色 (Specular Color) 贴图

* **作用:** 此贴图定义了表面**高光反射的颜色和强度**。基础的 Blinn-Phong 模型通常使用一个统一的 `specularColor`。但现实中，不同材质的高光颜色不同（例如，金属的高光通常带有金属本身的颜色，而绝缘体的高光通常是白色）。Specular 贴图允许我们逐像素地控制这一点。
* **实现方式:** 在片元着色器中，如果 Specular 贴图存在，我们就采样它来获取当前片元的高光颜色 `mapSpecularColor`，并**用它替代**统一的 `uniform_SpecularColor`。如果贴图不存在，则回退使用统一颜色。
    ```
    // 伪代码
    if (useSpecularMap) {
        matSpecular = sample(specularTexture, uv);
    } else {
        matSpecular = uniform_SpecularColor;
    }
    // ... 使用 matSpecular 计算高光 ...
    ```
* **视觉效果:** 可以表现混合材质，如金属上的锈迹（锈迹部分高光弱或无），或者带有特定颜色反射的材质。

### 2.3 光泽度 (Glossiness) 贴图

* **作用:** 光泽度贴图（有时也叫光滑度 Smoothness 图，或者反过来用粗糙度 Roughness 图）控制表面**高光的锐利程度**。光滑的表面（如镜子、抛光金属）有小而亮的高光，而粗糙的表面（如磨砂塑料、石头）则有模糊而散开的高光。
* **实现方式:** 在 Blinn-Phong 模型中，高光的锐利程度由 `shininess` 指数控制（值越高，高光越小越亮）。Gloss 贴图通常是**灰度图**，其值（范围 0.0 到 1.0）需要**映射**到一个合适的 `shininess` 范围。例如，可以将 Gloss 值 0.0 映射到最低 `shininess`（如 2），将 1.0 映射到最高 `shininess`（如 256 或更高）。
    ```
    // 伪代码
    if (useGlossMap) {
        glossFactor = sample(glossTexture, uv).r; // 取单通道
        // 线性映射示例
        currentShininess = lerp(MIN_SHININESS, MAX_SHININESS, glossFactor);
    } else {
        currentShininess = uniform_Shininess;
    }
    // ... specFactor = pow(NdotH, currentShininess) ...
    ```
    注意：从 Gloss 值到 Shininess 的映射关系可以根据需要调整，线性、指数或自定义曲线都可以。
* **视觉效果:** 极大地增强了材质的区分度，能清晰地表现出物体表面的光滑或粗糙程度。

## 3. 代码实现要点

将这些贴图集成到我们现有的渲染器中，主要涉及以下修改：

### 3.1 数据结构 (`Material`, `Shader`)

* 在 `Material` 结构体中添加 `aoTexture`, `specularTexture`, `glossTexture` 成员（类型为 `Texture`）以及对应的加载函数。
* 在 `Shader` 基类中添加对应的 `uniform_AoTexture`, `uniform_SpecularTexture`, `uniform_GlossTexture` uniform 变量，以及 `uniform_UseAoMap`, `uniform_UseSpecularMap`, `uniform_UseGlossMap` 的布尔标志。

```c++
// include/core/material.h (部分)
struct Material {
    // ... (之前的成员) ...
    Texture aoTexture;
    Texture specularTexture;
    Texture glossTexture;
    // ... (加载函数) ...
};

// include/core/shader.h (部分)
class Shader {
public:
    // ... (之前的 Uniforms) ...
    Texture uniform_AoTexture;
    bool uniform_UseAoMap = false;
    Texture uniform_SpecularTexture;
    bool uniform_UseSpecularMap = false;
    Texture uniform_GlossTexture;
    bool uniform_UseGlossMap = false;
    // ...
};
```

### 3.2 渲染器 (Renderer)
在 Renderer::drawModel 函数中设置 Shader Uniform 的部分，添加对新贴图和标志的设置：

```cpp
// src/core/renderer.cpp (drawModel 部分)
void Renderer::drawModel(Model& model, const mat4& modelMatrix, const Material& material) {
    // ... (检查 shader) ...
    auto& shader = *material.shader;
    // ... (设置矩阵、光照、基础材质 Uniforms) ...

    // 设置新贴图 Uniforms 和 Flags
    shader.uniform_AoTexture = material.aoTexture;
    shader.uniform_UseAoMap = !material.aoTexture.empty();

    shader.uniform_SpecularTexture = material.specularTexture;
    shader.uniform_UseSpecularMap = !material.specularTexture.empty();

    shader.uniform_GlossTexture = material.glossTexture;
    shader.uniform_UseGlossMap = !material.glossTexture.empty();

    // ... (顶点处理与渲染循环) ...
}
```

### 3.3 片元着色器 (Fragment Shader)
这是改动最大的地方，在 BlinnPhongShader::fragment 中集成新贴图的采样和应用逻辑：

```cpp
// src/core/blinn_phong_shader.cpp (fragment 部分)
bool BlinnPhongShader::fragment(const Varyings& input, vec3f& outColor) {
    // --- 获取法线 N (处理法线贴图) ---
    // ... (同上一篇文章) ...

    // --- 获取材质属性 (考虑贴图) ---
    // Diffuse Color
    vec3f matDiffuse = uniform_DiffuseColor;
    if (uniform_UseDiffuseMap && !uniform_DiffuseTexture.empty()) { /* modulate */ }

    // Specular Color
    vec3f matSpecular = uniform_SpecularColor; // Default
    if (uniform_UseSpecularMap && !uniform_SpecularTexture.empty()) {
        matSpecular = uniform_SpecularTexture.sample(input.uv.x, input.uv.y); // Override
    }

    // Shininess (via Gloss Map)
    int currentShininess = uniform_Shininess; // Default
    if (uniform_UseGlossMap && !uniform_GlossTexture.empty()) {
        float glossFactor = uniform_GlossTexture.sample(input.uv.x, input.uv.y).x; // Sample gloss (e.g., R channel)
        glossFactor = std::max(0.0f, std::min(1.0f, glossFactor));
        const int minShininess = 2;
        const int maxShininess = 256; // Adjust range as needed
        currentShininess = minShininess + static_cast<int>(static_cast<float>(maxShininess - minShininess) * glossFactor);
        currentShininess = std::max(minShininess, currentShininess);
    }

    // --- AO Factor ---
    float aoFactor = 1.0f; // Default: full ambient
    if (uniform_UseAoMap && !uniform_AoTexture.empty()) {
        aoFactor = uniform_AoTexture.sample(input.uv.x, input.uv.y).x; // Sample AO (e.g., R channel)
        aoFactor = std::max(0.0f, std::min(1.0f, aoFactor));
    }

    // --- 光照计算 ---
    vec3f V = (uniform_CameraPosition - input.worldPosition).normalized();
    vec3f matAmbient = uniform_AmbientColor;

    // Ambient Term (modulated by AO)
    vec3f ambientTerm = uniform_AmbientLight * matAmbient * aoFactor;
    vec3f totalColor = ambientTerm;

    // 循环处理光源
    for (const auto& light : uniform_Lights) {
        // ... (计算 L, lightCol, attenuation) ...

        // Diffuse Term
        float NdotL = std::max(0.0f, N.dot(L));
        vec3f diffuse = matDiffuse * lightCol * NdotL * attenuation;

        // Specular Term (using derived matSpecular and currentShininess)
        vec3f H = (L + V).normalized();
        float NdotH = std::max(0.0f, N.dot(H));
        float specFactor = fastPow(NdotH, currentShininess); // Use mapped shininess
        vec3f specular = matSpecular * lightCol * specFactor * attenuation; // Use mapped specular color

        totalColor = totalColor + diffuse + specular;
    }

    // --- Final Color ---
    // ... (Clamp outColor) ...
    return true;
}
```

### 4. 效果展示
当这几种贴图组合在一起时，渲染结果的真实感将得到显著提升。金属部分会呈现出带有颜色的高光，锈迹部分则显得暗淡粗糙；模型缝隙的阴影感更强，整体光照更加自然。

{% asset_img detailed_maps_comparison.png Comparison with Detailed Maps %}
（示意图：对比仅有 Diffuse/Normal 与 包含 AO/Specular/Gloss 的渲染效果）

### 5. 总结与展望
通过引入 AO、Specular 和 Gloss 贴图，我们的软渲染器在表现材质细节方面迈进了一大步。这使得我们能够更精细地控制光照的各个方面，模拟出更加多样和逼真的表面效果。

这些贴图的概念实际上也是基于物理的渲染 (Physically Based Rendering - PBR) 工作流的核心组成部分（尽管 PBR 通常使用不同的参数组合，如 Albedo、Metallic、Roughness、AO）。虽然我们当前的 Blinn-Phong 光照模型并非严格意义上的 PBR，但对这些贴图的支持为将来向更先进的 PBR 光照模型迁移打下了良好的基础。

下一步，可以考虑实现更复杂的 PBR 光照模型（如 Cook-Torrance），或者引入环境贴图 (Environment Mapping) 来实现基于图像的光照 (Image-Based Lighting - IBL)，让渲染效果更上一层楼。