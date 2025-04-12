---
title: 在软渲染器中实现法线贴图 (Normal Mapping)
date: 2025-04-12 22:33:40
tags:
  - 图形学
  - 渲染
  - 法线贴图
  - C++
  - 软渲染器
categories:
  - 技术分享
---

在实时计算机图形学中，模型的细节往往受到多边形数量的限制。为了在不显著增加模型复杂度的前提下，模拟出丰富的表面细节（如凹凸、划痕、纹理），法线贴图技术应运而生。本文将详细介绍如何在基于 C++ 的软件渲染器中实现切线空间法线贴图 (Tangent Space Normal Mapping)。

## 1. 问题的提出：低模的局限性

传统的低多边形模型 (Low-Poly Model) 依赖于顶点法线 (Vertex Normals) 进行光照计算。通过 Gouraud Shading 或 Phong Shading，我们可以在顶点之间插值法线，获得平滑的光照过渡效果。然而，这种方法无法表现模型表面的微小几何细节。如果想要模型拥有丰富的凹凸细节，就需要极高数量的多边形，这对于实时渲染来说通常是不可接受的。

{% asset_img normal_mapping_comparison.png Comparison %}
*（示意图：左侧为低模+顶点法线光照，右侧为低模+法线贴图光照）*

## 2. 解决方案：法线贴图

法线贴图的核心思想是：**用一张纹理来存储模型表面各点的法线信息**。这张特殊的纹理被称为“法线贴图”。在渲染时，我们不再直接使用插值得到的顶点法线，而是从法线贴图中采样对应片元 (Fragment) 的法线向量，并用这个采样得到的法线来进行光照计算。

由于纹理可以存储非常丰富的信息，即使模型本身多边形数量很少，通过法线贴图也能模拟出极其逼真的表面细节。

## 3. 关键概念：切线空间 (Tangent Space)

直接将世界空间 (World Space) 或模型空间 (Model Space) 的法线存储在纹理中是可行的，但这会导致法线贴图与模型的特定姿态或变换绑定，难以复用。更常用的方法是使用 **切线空间 (Tangent Space)**。

切线空间是一个**局部坐标系**，定义在模型的每个表面点上。它由三个相互正交（或近似正交）的基向量构成：

1.  **法线 (Normal - N):** 即该点的原始顶点法线，通常垂直于表面。
2.  **切线 (Tangent - T):** 平行于表面，通常沿着纹理坐标 U 的增加方向。
3.  **副切线 (Bitangent - B):** 平行于表面，通常沿着纹理坐标 V 的增加方向，并且可以通过 `N` 和 `T` 的叉乘得到 (`B = cross(N, T)`) 来保证正交性。

{% asset_img tangent_space_basis.png TBN Basis %}
*（示意图：模型表面一点的切线空间 TBN 基向量）*

法线贴图中存储的是**相对于这个局部 TBN 坐标系的法线扰动**。通常，RGB 通道对应 TBN 向量：
* R -> Tangent 方向分量
* G -> Bitangent 方向分量
* B -> Normal 方向分量

一个“平坦”表面的法线在切线空间中通常是 `(0, 0, 1)`。由于颜色通道通常存储在 `[0, 1]` 范围内，而法线分量在 `[-1, 1]` 范围内，因此需要进行映射。常用的映射方式为：

存储值 = (法线分量 + 1.0) / 2.0


或者反过来，从纹理采样值恢复法线分量：

法线分量 = 采样值 * 2.0 - 1.0


因此，法线贴图中常见的“基准”蓝色 `(0.5, 0.5, 1.0)` 就代表了切线空间中的 `(0, 0, 1)` 法线，即未发生扰动的原始表面法线方向。

**使用切线空间的好处：**

* **解耦:** 法线信息与模型的具体旋转、变形无关。
* **复用:** 同一张法线贴图可以应用在不同模型或模型的不同部分（只要它们的 UV 布局允许）。
* **压缩友好:** 大部分法线的 Z 分量（Normal 方向）都接近 1，可以通过优化存储。

## 4. 实现步骤

要在我们的软渲染器中实现切线空间法线贴图，需要修改渲染管线的多个阶段。

### 4.1 计算顶点切线和副切线

我们需要为模型的每个顶点计算其 TBN 基础向量。这通常在模型加载后、渲染前完成。计算方法基于构成三角形的顶点位置和纹理坐标：

对于三角形 `P0, P1, P2` 及其对应的纹理坐标 `UV0, UV1, UV2`：

1.  计算边向量：
    * `Edge1 = P1 - P0`
    * `Edge2 = P2 - P0`
2.  计算 UV 差量：
    * `DeltaUV1 = UV1 - UV0`
    * `DeltaUV2 = UV2 - UV0`
3.  计算系数 `f`：
    * `f = 1.0 / (DeltaUV1.x * DeltaUV2.y - DeltaUV2.x * DeltaUV1.y)`
4.  计算切线 `T` 和副切线 `B`：
    * `Tangent.x = f * (DeltaUV2.y * Edge1.x - DeltaUV1.y * Edge2.x)`
    * `Tangent.y = f * (DeltaUV2.y * Edge1.y - DeltaUV1.y * Edge2.y)`
    * `Tangent.z = f * (DeltaUV2.y * Edge1.z - DeltaUV1.y * Edge2.z)`
    * `Bitangent.x = f * (-DeltaUV2.x * Edge1.x + DeltaUV1.x * Edge2.x)`
    * `Bitangent.y = f * (-DeltaUV2.x * Edge1.y + deltaUV1.x * Edge2.y)`
    * `Bitangent.z = f * (-DeltaUV2.x * Edge1.z + deltaUV1.x * Edge2.z)`

计算出的 `T` 和 `B` 需要累加到每个顶点上（因为一个顶点可能被多个三角形共享）。

最后，对每个顶点的 `T` 和 `B` 进行**正交化和归一化**处理，常用 Gram-Schmidt 方法：

1.  `T = normalize(T - N * dot(N, T))`  // 使 T 正交于 N
2.  检查 `dot(cross(N, T), B)` 的符号，判断 TBN 坐标系的左右手性是否与 UV 坐标系一致，必要时翻转 T。
3.  `B = normalize(cross(N, T))`      // 重新计算 B 以确保正交

**代码片段 (Model::calculateTangents):**
```c++
// src/core/model.cpp
void Model::calculateTangents() {
    tangents.assign(numVertices(), vec3f(0.0f, 0.0f, 0.0f));
    bitangents.assign(numVertices(), vec3f(0.0f, 0.0f, 0.0f));

    // --- Loop through faces to calculate T and B contributions ---
    for (size_t i = 0; i < numFaces(); ++i) {
        // ... (Get vertices v0, v1, v2 and uvs uv0, uv1, uv2) ...
        vec3f edge1 = v1 - v0;
        vec3f edge2 = v2 - v0;
        vec2f deltaUV1 = uv1 - uv0;
        vec2f deltaUV2 = uv2 - uv0;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
         if (std::isinf(f) || std::isnan(f)) { f = 0.0f; } // Avoid NaN/Inf

        vec3f tangent = (edge1 * deltaUV2.y - edge2 * deltaUV1.y) * f;
        vec3f bitangent = (edge2 * deltaUV1.x - edge1 * deltaUV2.x) * f;

        // Accumulate for vertices
        for (int j = 0; j < 3; ++j) {
            tangents[face.vertIndex[j]] = tangents[face.vertIndex[j]] + tangent;
            bitangents[face.vertIndex[j]] = bitangents[face.vertIndex[j]] + bitangent;
        }
    }

    // --- Loop through vertices to orthogonalize and normalize ---
    for (size_t i = 0; i < numVertices(); ++i) {
        const vec3f& n = getNormal(i); // Assuming normal indices match vertex indices after processing
        vec3f& t = tangents[i];
        vec3f& b = bitangents[i];

        if (t.length() > 1e-6f && n.length() > 1e-6f) {
            // Gram-Schmidt orthogonalize T against N
            t = (t - n * n.dot(t)).normalized();

            // Check handedness and recalculate B
            if (n.cross(t).dot(b) < 0.0f) {
                t = t * -1.0f; // Flip tangent if needed
            }
            b = n.cross(t).normalized(); // Ensure B is orthogonal and normalized
        } else {
            // Handle degenerate cases: Create arbitrary orthogonal basis
             vec3f up = (std::abs(n.y) < 0.99f) ? vec3f(0.0f, 1.0f, 0.0f) : vec3f(1.0f, 0.0f, 0.0f);
             t = n.cross(up).normalized();
             b = n.cross(t).normalized();
        }
         // Fallback for NaN/Inf safety
        if (std::isnan(t.x) || std::isinf(t.x)) t = vec3f(1,0,0);
        if (std::isnan(b.x) || std::isinf(b.x)) b = vec3f(0,0,1);
    }
}
```

将计算得到的 tangents 和 bitangents 存储在 Model 类中。

### 4.2 数据准备与传递

* Material: 在 Material 结构体中添加 normalTexture 成员及加载方法。
* Shader Uniforms: 在 Shader 基类中添加 uniform_NormalTexture (类型 Texture) 和 uniform_UseNormalMap (类型 bool)。
* Vertex Input: 修改 VertexInput 结构体，添加 tangent 和 bitangent 成员。

```cpp
// include/core/shader.h
struct VertexInput {
    vec3f position;
    vec3f normal;
    vec2f uv;
    vec3f tangent;   // Added
    vec3f bitangent; // Added
};
```

* Varyings: 修改 Varyings 结构体，传递世界空间下的 TBN 基向量。
```cpp
// include/core/shader.h
struct Varyings {
    vec4f clipPosition;
    vec3f worldPosition;
    vec2f uv;
    // World-space TBN basis vectors
    vec3f tangent;   // World Tangent
    vec3f bitangent; // World Bitangent
    vec3f normal;    // World (Geometric) Normal
};
```
* Renderer: 在 Renderer::drawModel 中，设置 uniform_NormalTexture 和 uniform_UseNormalMap。在构建 VertexInput 时，从 Model 获取 tangent 和 bitangent。

### 4.3 顶点着色器 (Vertex Shader)
顶点着色器的主要任务是将 TBN 基向量从模型空间转换到世界空间，并传递给片元着色器。

代码片段 (BlinnPhongShader::vertex):
```cpp
// src/core/blinn_phong_shader.cpp
Varyings BlinnPhongShader::vertex(const VertexInput& input) {
    Varyings output;
    vec4f modelPos4(input.position, 1.0f);
    vec4f modelNormal4(input.normal, 0.0f);
    vec4f modelTangent4(input.tangent, 0.0f);
    vec4f modelBitangent4(input.bitangent, 0.0f);

    // Calculate world position
    output.worldPosition = (uniform_ModelMatrix * modelPos4).xyz();

    // Transform TBN vectors to world space using Normal Matrix
    // uniform_NormalMatrix is typically transpose(inverse(ModelMatrix))
    // Ensure they are normalized after transformation.
    output.normal    = (uniform_NormalMatrix * modelNormal4).xyz().normalized();
    output.tangent   = (uniform_NormalMatrix * modelTangent4).xyz().normalized();
    output.bitangent = (uniform_NormalMatrix * modelBitangent4).xyz().normalized();
    // Optional: Recalculate bitangent worldB = cross(worldN, worldT) here for robustness.

    // Pass UVs
    output.uv = input.uv;

    // Calculate clip space position
    output.clipPosition = uniform_MVP * modelPos4;

    return output;
}
```

### 4.4 片元着色器 (Fragment Shader)
片元着色器是实现法线贴图的核心：

1. 检查是否使用法线贴图: 根据 法线的 texture 是否为空为标志。
2. 采样法线贴图: 如果使用，则根据插值得到的 uv 坐标采样 uniform_NormalTexture。
3. 解压法线: 将采样到的 [0, 1] 颜色值转换回 [-1, 1] 的切线空间法线向量 N_{tangent}。
    * N_{tangent} = normalize(Sample_{RGB} * 2.0 - 1.0)
4. 构建 TBN 矩阵: 使用从顶点着色器插值得到的世界空间 TBN 基向量（需要重新归一化）。
    * T = normalize(input.tangent)
    * B = normalize(input.bitangent)
    * N_{geom} = normalize(input.normal)
5. 转换法线: 将切线空间法线 N_{tangent} 转换到世界空间。
    * N_{world} = normalize(T * N_{tangent}.x + B * N_{tangent}.y + N_{geom} * N_{tangent}.z)
6. 光照计算: 使用计算得到的 N_{world} (如果使用了法线贴图) 或 N_{geom} (如果未使用) 进行后续的 Blinn-Phong 或其他光照模型计算。

代码片段 (BlinnPhongShader::fragment):

```cpp
// src/core/blinn_phong_shader.cpp
bool BlinnPhongShader::fragment(const Varyings& input, vec3f& outColor) {
    vec3f N; // The final normal used for lighting

    if (uniform_UseNormalMap && !uniform_NormalTexture.empty()) {
        // 1. Sample the normal map
        vec3f tangentNormalSample = uniform_NormalTexture.sample(input.uv.x, input.uv.y);

        // 2. Unpack from [0,1] to [-1,1] and normalize
        vec3f tangentNormal = (tangentNormalSample * 2.0f) - vec3f(1.0f, 1.0f, 1.0f);
        tangentNormal = tangentNormal.normalized(); // Ensure unit length

        // 3. Get interpolated world-space TBN basis (renormalize)
        vec3f T = input.tangent.normalized();
        vec3f B = input.bitangent.normalized();
        vec3f N_geom = input.normal.normalized();

        // 4. Transform tangent-space normal to world space
        // N_world = T*Nx_tan + B*Ny_tan + N_geom*Nz_tan
        N = T * tangentNormal.x + B * tangentNormal.y + N_geom * tangentNormal.z;
        N = N.normalized(); // Final normal for lighting
    } else {
        // Use interpolated geometric normal if no normal map
        N = input.normal.normalized();
    }

    // --- Proceed with Blinn-Phong lighting using the final Normal N ---
    vec3f V = (uniform_CameraPosition - input.worldPosition).normalized(); // View direction
    vec3f totalColor = uniform_AmbientLight * uniform_AmbientColor; // Start with ambient

    // Get material properties (potentially textured)
    vec3f matDiffuse = uniform_DiffuseColor;
    if (!uniform_DiffuseTexture.empty()) {
        matDiffuse = matDiffuse * uniform_DiffuseTexture.sample(input.uv.x, input.uv.y);
    }
    // ... (Get matSpecular, matShininess) ...

    for (const auto& light : uniform_Lights) {
        // ... (Calculate L, lightCol, attenuation) ...

        // Diffuse
        float NdotL = std::max(0.0f, N.dot(L));
        vec3f diffuse = matDiffuse * lightCol * NdotL * attenuation;

        // Specular (Blinn-Phong)
        vec3f H = (L + V).normalized();
        float NdotH = std::max(0.0f, N.dot(H));
        float specFactor = fastPow(NdotH, uniform_Shininess); // Use the fastPow utility
        vec3f specular = uniform_SpecularColor * lightCol * specFactor * attenuation;

        totalColor = totalColor + diffuse + specular;
    }

    // Clamp final color
    outColor.x = std::min(1.0f, std::max(0.0f, totalColor.x));
    outColor.y = std::min(1.0f, std::max(0.0f, totalColor.y));
    outColor.z = std::min(1.0f, std::max(0.0f, totalColor.z));

    return true; // Pixel should be written
}
```


### 4.5 插值 (Interpolation)
确保 Renderer::interpolateVaryings 函数能够正确地对新增的 tangent, bitangent, normal 向量进行透视矫正插值。由于 interpolateVaryings 内部使用了模板化的 perspectiveCorrectInterpolate，只需要在 interpolateVaryings 中添加对这三个新成员的调用即可。

```cpp
// src/core/renderer.cpp
Varyings Renderer::interpolateVaryings(float t, const Varyings& start, const Varyings& end, float startInvW, float endInvW) const {
    Varyings result;
    // ... (Interpolate worldPosition, uv) ...
    result.normal = perspectiveCorrectInterpolate(t, start.normal, end.normal, startInvW, endInvW);
    result.tangent = perspectiveCorrectInterpolate(t, start.tangent, end.tangent, startInvW, endInvW);
    result.bitangent = perspectiveCorrectInterpolate(t, start.bitangent, end.bitangent, startInvW, endInvW);
    return result;
}
```

### 5. 总结与效果
通过以上步骤，我们就成功地在软渲染器中集成了切线空间法线贴图。渲染低多边形模型时，通过在片元着色器中查询法线贴图并使用得到的法线进行光照计算，可以在几乎不增加几何复杂度的前提下，极大地提升模型的表面细节和真实感。

这项技术是现代实时渲染中不可或缺的一部分，能够以较低的性能开销实现高质量的视觉效果。

### 6. 注意事项
* Tangent Calculation: 上述切线计算方法比较基础，对于复杂的 UV 布局或重叠 UV 可能产生问题。更精确的方法（如 MikkTSpace）更为健壮。
* Normal Map Format: 注意法线贴图的 Y 分量（通常是绿色通道）在不同规范（如 OpenGL 和 DirectX）中可能方向相反。需要确保加载和解压时使用正确的约定。
* TBN 正交性: 插值后的 TBN 基向量可能不再严格正交，在片元着色器中重新正交化（如通过 B = cross(N, T)）可以提高精度，但会增加计算量。
* sRGB: 如果法线贴图被错误地当作 sRGB 纹理处理，会导致解压出的法线不准确。应确保法线贴图作为线性数据处理。