---
title: Mipmap 实现详解：从原理到代码（含数学推导）
date: 2025-04-25
tags: [Graphics, Rendering, Mipmap, Texture Filtering, LOD]
categories: [Computer Graphics]
---

在实时渲染中，纹理贴图是赋予模型表面细节的关键技术。然而，当一个带有高分辨率纹理的模型距离摄像机很远，或者以一个倾斜的角度观察时，屏幕上的一个像素可能会对应纹理上的多个纹素（Texel）。如果不进行处理，直接采样会导致严重的**摩尔纹（Moiré patterns）**和**闪烁（Shimmering）**现象，即**纹理混叠（Texture Aliasing）**。

Mipmapping 是解决这一问题的经典技术。其核心思想是预先生成一系列分辨率递减的纹理版本（Mip 层级），并在渲染时根据屏幕像素所需的细节程度（Level of Detail, LOD）选择合适的 Mip 层级进行采样，从而有效减少混叠并提高渲染性能。

本文将详细阐述 Mipmap 的实现过程，包括 Mipmap 的生成、关键的 LOD 计算（包含数学推导）以及最终的三线性过滤采样。

## 1. Mipmap 的生成

Mipmap 的基础是创建一系列低分辨率的纹理图像：

- **Level 0**: 原始的、最高分辨率的纹理。
- **Level 1**: 分辨率是 Level 0 的一半（宽和高各一半）。
- **Level 2**: 分辨率是 Level 1 的一半。
- ... 以此类推，直到某个维度的分辨率达到 1。

### 生成方法

最常用的方法是**下采样（Downsampling）**。一个简单的实现是使用 **2x2 盒子滤波器（Box Filter）**：将上一层级中每 2x2 个像素的颜色进行平均，得到下一层级的一个像素颜色。

以下是 `TGATexture` 实现中的 `generateNextMipLevel` 函数示例：

```c++
namespace { // Anonymous namespace

// Simple Box Filter downsampling for Mipmap Generation
bool generateNextMipLevel(const Texture::MipLevel& inputLevel, Texture::MipLevel& outputLevel) {
    if (inputLevel.width <= 1 && inputLevel.height <= 1) {
        return false; // Cannot downsample further
    }

    outputLevel.width = std::max(1, inputLevel.width / 2);
    outputLevel.height = std::max(1, inputLevel.height / 2);
    outputLevel.pixels.resize(outputLevel.width * outputLevel.height);

    for (int y = 0; y < outputLevel.height; ++y) {
        for (int x = 0; x < outputLevel.width; ++x) {
            // Calculate corresponding 2x2 area top-left corner in input level
            int inputX = x * 2;
            int inputY = y * 2;

            // Average 2x2 block (handle boundaries by clamping coordinates)
            const vec3f& p00 = inputLevel.pixels[std::min(inputLevel.height - 1, inputY + 0) * inputLevel.width + std::min(inputLevel.width - 1, inputX + 0)];
            const vec3f& p10 = inputLevel.pixels[std::min(inputLevel.height - 1, inputY + 0) * inputLevel.width + std::min(inputLevel.width - 1, inputX + 1)];
            const vec3f& p01 = inputLevel.pixels[std::min(inputLevel.height - 1, inputY + 1) * inputLevel.width + std::min(inputLevel.width - 1, inputX + 0)];
            const vec3f& p11 = inputLevel.pixels[std::min(inputLevel.height - 1, inputY + 1) * inputLevel.width + std::min(inputLevel.width - 1, inputX + 1)];

            vec3f sumColor = (p00 + p10 + p01 + p11) * 0.25f; // Average the 4 pixels
            outputLevel.pixels[y * outputLevel.width + x] = sumColor;
        }
    }
    return true;
}

} // end anonymous namespace
```

在 `TGATexture::load` 中，生成 Mipmap 的流程如下：

```c++
// Inside TGATexture::load after loading base level
int currentLevelIndex = 0;
while (mipLevels[currentLevelIndex].width > 1 || mipLevels[currentLevelIndex].height > 1) {
    Texture::MipLevel nextLevel;
    if (!generateNextMipLevel(mipLevels[currentLevelIndex], nextLevel)) break;
    mipLevels.push_back(std::move(nextLevel));
    currentLevelIndex++;
    // Safety break ...
}
```

### 加载预生成 Mipmap

某些纹理格式（如 DDS）允许直接存储预先生成好的 Mipmap 层级。加载时只需按顺序读取并解压（如果需要）每个层级的数据：

```c++
// Inside DDSTexture::load
uint32_t numLevels = (header.flags & DDSD_MIPMAPCOUNT) ? header.mipMapCount : 1;
mipLevels.resize(numLevels);
int currentWidth = baseWidth;
int currentHeight = baseHeight;

for (uint32_t level = 0; level < numLevels; ++level) {
    // ... calculate dataSize for this level ...
    std::vector<unsigned char> compressedData(dataSize);
    file.read(reinterpret_cast<char*>(compressedData.data()), dataSize);
    // ... check read errors ...

    mipLevels[level].width = currentWidth;
    mipLevels[level].height = currentHeight;

    // Decompress this level into mipLevels[level].pixels
    bool success = false;
    if (isDXT1) success = decompressDXT1LevelInternal(compressedData, currentWidth, currentHeight, mipLevels[level].pixels);
    // ... handle DXT5, ATI2 etc. ...
    if (!success) { /* handle error */ }

    // Calculate dimensions for the next level
    currentWidth = std::max(1, currentWidth / 2);
    currentHeight = std::max(1, currentHeight / 2);
}
```

## 2. 细节级别 (Level of Detail - LOD) 计算

LOD 计算是 Mipmapping 的核心。我们需要为屏幕上的每个像素计算一个 LOD 值，表示该像素需要多大程度的纹理细节。LOD 值越高，表示需要的细节越少，应使用分辨率更低的 Mip 层级。

LOD 的计算基于纹理坐标在屏幕空间的变化率。如果纹理坐标 $(u, v)$ 相对于屏幕坐标 $(x, y)$ 变化很快（例如，纹理被强烈压缩），则需要更模糊的 Mip 层级（高 LOD）；反之，如果变化很慢（纹理被放大），则需要更清晰的 Mip 层级（低 LOD）。

### 数学推导

目标是计算偏导数：$\frac{\partial u}{\partial x}$, $\frac{\partial u}{\partial y}$, $\frac{\partial v}{\partial x}$, $\frac{\partial v}{\partial y}$。由于透视投影的存在，$u$ 和 $v$ 并非屏幕坐标 $x$ 和 $y$ 的线性函数，直接计算这些偏导数较为复杂。

经过透视除法后的**透视矫正（Perspective-Correct）**属性是屏幕坐标的线性函数。这些属性包括 $u' = \frac{u}{w}$, $v' = \frac{v}{w}$ 以及 $q = \frac{1}{w}$，其中 $w$ 是顶点变换到裁剪空间后的齐次坐标 $W$ 分量。

我们可以先计算这些矫正后属性对屏幕坐标的梯度：$\frac{\partial u'}{\partial x}$, $\frac{\partial u'}{\partial y}$, $\frac{\partial v'}{\partial x}$, $\frac{\partial v'}{\partial y}$, $\frac{\partial q}{\partial x}$, $\frac{\partial q}{\partial y}$。这些梯度在三角形内部是恒定的，可在三角形设置阶段（光栅化之前）计算一次。

假设三角形在屏幕空间的顶点坐标为 $(x_0, y_0), (x_1, y_1), (x_2, y_2)$，对应的某个透视矫正属性值为 $a_0, a_1, a_2$。我们可以建立线性方程组：

$$
\begin{aligned}
a_0 &= A x_0 + B y_0 + C \\
a_1 &= A x_1 + B y_1 + C \\
a_2 &= A x_2 + B y_2 + C
\end{aligned}
$$

解这个方程组得到 $A = \frac{\partial a}{\partial x}$ 和 $B = \frac{\partial a}{\partial y}$。使用克莱姆法则或直接代入消元：

$$
\frac{\partial a}{\partial x} = \frac{(a_1 - a_0)(y_2 - y_0) - (a_2 - a_0)(y_1 - y_0)}{(x_1 - x_0)(y_2 - y_0) - (x_2 - x_0)(y_1 - y_0)}
$$

$$
\frac{\partial a}{\partial y} = \frac{(a_2 - a_0)(x_1 - x_0) - (a_1 - a_0)(x_2 - x_0)}{(x_1 - x_0)(y_2 - y_0) - (x_2 - x_0)(y_1 - y_0)}
$$

分母是三角形屏幕空间面积的两倍（带符号）。令 $\Delta = (x_1 - x_0)(y_2 - y_0) - (x_2 - x_0)(y_1 - y_0)$。将 $a$ 替换为 $u', v', q$，即可计算 $\frac{\partial u'}{\partial x}$, $\frac{\partial v'}{\partial x}$, $\frac{\partial q}{\partial x}$ 等。

以下是相关代码实现：

```c++
// Inside calculateAccurateGradients(const ScreenVertex v[3])
AccurateScreenSpaceGradients grads;
float x0 = static_cast<float>(v[0].x), y0 = static_cast<float>(v[0].y);
// ... x1, y1, x2, y2 ...
vec2f uv0_over_w = v[0].varyings.uv * v[0].invW;
// ... uv1_over_w, uv2_over_w ...
float invW0 = v[0].invW; // This is q0
// ... invW1, invW2 ...

float delta = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0);
if (std::abs(delta) < 1e-9f) return grads; // Handle degenerate
float invDelta = 1.0f / delta;

// Calculate gradients of perspective-correct attributes
grads.dUVoverW_dX = ((uv1_over_w - uv0_over_w) * (y2 - y0) - (uv2_over_w - uv0_over_w) * (y1 - y0)) * invDelta;
grads.dUVoverW_dY = ((uv2_over_w - uv0_over_w) * (x1 - x0) - (uv1_over_w - uv0_over_w) * (x2 - x0)) * invDelta;
grads.dInvW_dX = ((invW1 - invW0) * (y2 - y0) - (invW2 - invW0) * (y1 - y0)) * invDelta; // dq/dx
grads.dInvW_dY = ((invW2 - invW0) * (x1 - x0) - (invW1 - invW0) * (x2 - x0)) * invDelta; // dq/dy
```

使用**链式法则**计算原始纹理坐标 $(u, v)$ 的导数。因为 $u = \frac{u'}{q}$ 且 $v = \frac{v'}{q}$：

$$
\frac{\partial u}{\partial x} = \frac{\partial}{\partial x} \left( \frac{u'}{q} \right) = \frac{\frac{\partial u'}{\partial x} q - u' \frac{\partial q}{\partial x}}{q^2} = \frac{1}{q} \frac{\partial u'}{\partial x} - \frac{u'}{q^2} \frac{\partial q}{\partial x} = w \frac{\partial u'}{\partial x} - u w \frac{\partial q}{\partial x}
$$

$$
\frac{\partial v}{\partial x} = w \frac{\partial v'}{\partial x} - v w \frac{\partial q}{\partial x}
$$

$$
\frac{\partial u}{\partial y} = w \frac{\partial u'}{\partial y} - u w \frac{\partial q}{\partial y}
$$

$$
\frac{\partial v}{\partial y} = w \frac{\partial v'}{\partial y} - v w \frac{\partial q}{\partial y}
$$

这些计算需在每个像素执行，因为 $u, v, w$（以及 $q = \frac{1}{w}$）在像素间通过插值得到：

```c++
// Inside drawScanlines pixel loop (x loop)
float currentInvW = invWa + (invWb - invWa) * tHoriz; // Interpolated q = 1/w
if (std::abs(currentInvW) < 1e-9f) continue; // Avoid division by zero
float currentW = 1.0f / currentInvW;
Varyings finalVaryings = interpolateVaryings(tHoriz, varyingsA, varyingsB, invWa, invWb); // Interpolated u, v etc.

// --- Accurate Derivative Calculation using Chain Rule ---
vec2f uv_ddx = currentW * gradients.dUVoverW_dX - finalVaryings.uv * currentW * gradients.dInvW_dX;
vec2f uv_ddy = currentW * gradients.dUVoverW_dY - finalVaryings.uv * currentW * gradients.dInvW_dY;
```

接下来，计算标量值 $\rho$，表示纹理在屏幕上被拉伸或压缩的程度。通常取 $x$ 和 $y$ 方向上变化率向量长度的最大值：

$$
\rho = \max \left( \sqrt{ \left( \frac{\partial u}{\partial x} \right)^2 + \left( \frac{\partial v}{\partial x} \right)^2 }, \sqrt{ \left( \frac{\partial u}{\partial y} \right)^2 + \left( \frac{\partial v}{\partial y} \right)^2 } \right)
$$

$\rho$ 表示屏幕上移动一个像素的距离，大约对应于纹理空间中移动 $\rho$ 个纹素的距离。最终，计算 LOD 值 $\lambda$（OpenGL 术语）：

$$
\lambda = \log_2(\rho)
$$

若 $\lambda = 0$，表示屏幕一个像素对应纹理一个纹素，使用 Level 0；若 $\lambda = 1$，表示屏幕一个像素对应纹理 2x2 个纹素，使用 Level 1；若 $\lambda = k$，表示屏幕一个像素对应纹理 $2^k \times 2^k$ 个纹素，使用 Level $k$。

在代码中，通常考虑纹理尺寸 $W_{tex}, H_{tex}$，并直接计算 $\rho^2$ 以避免开方：

$$
\rho^2 \approx \max \left( \frac{\left\| \frac{d(u,v)}{dx} \right\|^2}{W_{tex}^2}, \frac{\left\| \frac{d(u,v)}{dy} \right\|^2}{H_{tex}^2} \right)
$$

这里使用向量 $\frac{d(u,v)}{dx} = \left( \frac{\partial u}{\partial x}, \frac{\partial v}{\partial x} \right)$ 的长度平方，并假设纹理是各向同性的。LOD 计算为：

$$
\lambda = \frac{1}{2} \log_2(\rho^2)
$$

代码实现如下：

```c++
// Inside Texture::sample(..., const vec2f& ddx, const vec2f& ddy)
const auto& baseLevel = mipLevels[0];
float baseWidth = static_cast<float>(baseLevel.width);
float baseHeight = static_cast<float>(baseLevel.height);

// Calculate rho squared
float rho_sq = std::max(ddx.lengthSq() * baseWidth * baseWidth,
                        ddy.lengthSq() * baseHeight * baseHeight);

// Calculate LOD level
float lod = 0.0f;
if (rho_sq > 1e-9f) { // Avoid log(0)
    lod = 0.5f * std::log2(rho_sq);
}
lod = std::max(0.0f, lod); // Clamp LOD >= 0
```

## 3. Mipmap 采样 (三线性过滤)

计算出 LOD 值 $\lambda$ 后，使用三线性过滤（Trilinear Filtering）从 Mipmap 层级中采样颜色：

1. **选择层级**：根据 $\lambda$ 确定两个最接近的 Mip 层级：
   - $D_0 = \lfloor \lambda \rfloor$（向下取整）
   - $D_1 = D_0 + 1$
   确保 $D_0$ 和 $D_1$ 不超过最大 Mip 层级索引。

2. **层内双线性采样**：对 $D_0$ 和 $D_1$，使用纹理坐标 $(u, v)$ 进行**双线性过滤（Bilinear Filtering）**，得到颜色 $C_0$ 和 $C_1$。

```c++
// Texture::sampleBilinear(const MipLevel& level, float u, float v) helper function
// ... calculates x0, y0, u_frac, v_frac ...
// ... samples 4 neighbors c00, c10, c01, c11 with clamping ...
// Bilinear interpolation:
vec3f top = c00 * (1.0f - u_frac) + c10 * u_frac;
vec3f bottom = c01 * (1.0f - u_frac) + c11 * u_frac;
return top * (1.0f - v_frac) + bottom * v_frac;
```

3. **层间线性插值**：计算 $\lambda$ 的小数部分 $t = \lambda - \lfloor \lambda \rfloor$，在 $C_0$ 和 $C_1$ 之间进行线性插值，得到最终颜色 $C$：

$$
C = C_0 \times (1 - t) + C_1 \times t
$$

代码实现如下：

```c++
// Inside Texture::sample(...) after calculating lod
int maxLevel = static_cast<int>(mipLevels.size()) - 1;
int level0_idx = static_cast<int>(std::floor(lod));
level0_idx = std::min(level0_idx, maxLevel); // Clamp

// Sample from the first level using bilinear filtering
vec3f color0 = sampleBilinear(mipLevels[level0_idx], u, v);

// If we are at the highest LOD or only have one level, return bilinearly filtered result
if (level0_idx == maxLevel) {
    return color0;
}

// Get the second level index for trilinear interpolation
int level1_idx = level0_idx + 1; // Already clamped indirectly

// Sample from the second level using bilinear filtering
vec3f color1 = sampleBilinear(mipLevels[level1_idx], u, v);

// Calculate interpolation factor between the two levels
float level_t = lod - static_cast<float>(level0_idx); // Fractional part of LOD

// Trilinear interpolation
return color0 * (1.0f - level_t) + color1 * level_t;
```

## 4. 整合到渲染管线

Mipmap 流程在渲染器中的整合如下：

1. **顶点着色器**：
   - 处理顶点，输出裁剪空间坐标和需要插值的 Varyings（包括纹理坐标 $uv$）。

2. **三角形处理 (processFace)**：
   - 对顶点进行透视除法和视口变换，得到屏幕坐标 $(x, y)$ 和 $invW$。
   - 进行背面剔除。
   - 调用 `calculateAccurateGradients` 计算三角形的 $\frac{\partial (u/w)}{\partial x}$, $\frac{\partial (1/w)}{\partial x}$ 等梯度。
   - 调用 `drawTriangle`。

3. **三角形光栅化 (drawTriangle, drawScanlines)**：
   - 遍历三角形覆盖的像素。
   - 对每个像素，使用重心坐标或边插值计算插值后的 Varyings（包括 $uv$）和 $invW$。
   - 使用链式法则和预计算的梯度，计算像素的 $\frac{\partial u}{\partial x}$, $\frac{\partial v}{\partial x}$, $\frac{\partial u}{\partial y}$, $\frac{\partial v}{\partial y}$。
   - 调用片段着色器，传入插值后的 Varyings 和 UV 导数。

4. **片段着色器 (fragment)**：
   - 接收插值后的 Varyings 和 UV 导数 ($uv_ddx$, $uv_ddy$)。
   - 调用 `Texture::sample(u, v, uv_ddx, uv_ddy)` 执行 LOD 计算和三线性过滤，返回颜色。
   - 使用采样结果进行光照计算，输出最终像素颜色。

## 5. 结论

Mipmapping 是现代实时渲染中不可或缺的技术。通过预计算多级分辨率的纹理，并根据屏幕空间变化率智能选择合适的层级进行采样（通常使用三线性过滤），它显著减少纹理混叠现象，提高渲染图像质量，同时通过减少访问高分辨率纹理数据提升性能。