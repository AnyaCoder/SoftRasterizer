---
title: Diffuse Texture Loading Implementation
date: 2025-04-04
categories: Computer Graphics
tags: [Rendering, Texture Mapping]
---

## 概述

本文档详细记录了在 SoftRasterizer 项目中实现 diffuse 材质加载的全过程，解决了初始加载失败的问题，使得模型能够正确显示纹理效果。

## 核心修改

### 1. Texture 类扩展

```cpp
// 定义 Texture 类支持 TGA 加载
class Texture {
public:
    int width = 0;
    int height = 0;
    std::vector<vec3f> pixels;

    bool loadFromTGA(const std::string& filename);
    vec3f sample(float u, float v) const;
    
    bool empty() const {
        return pixels.empty() || width == 0 || height == 0;
    }
};

// 实现 TGA 文件加载
bool Texture::loadFromTGA(const std::string& filename) {
    std::vector<unsigned char> data;
    if (!loadTGA(filename, width, height, data)) {
        return false;
    }

    pixels.resize(width * height);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 3;
            pixels[y * width + x] = vec3f(
                data[idx] / 255.0f,     // R
                data[idx + 1] / 255.0f, // G
                data[idx + 2] / 255.0f  // B
            );
        }
    }
    return true;
}
```

### 2. 支持 RLE 压缩的 TGA 加载

```cpp
bool loadTGA(const std::string& filename, int& width, int& height, std::vector<unsigned char>& data) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;

    TGAHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));

    // 支持未压缩 (2) 和 RLE 压缩 (10) 的 24 位 RGB 图像
    if ((header.datatypecode != 2 && header.datatypecode != 10) || header.bitsperpixel != 24) {
        std::cerr << "Unsupported TGA format" << std::endl;
        return false;
    }

    width = header.width;
    height = header.height;
    data.resize(width * height * 3);

    file.seekg(header.idlength + header.colormaplength * (header.colormapdepth / 8), std::ios::cur);

    if (header.datatypecode == 2) {
        file.read(reinterpret_cast<char*>(data.data()), data.size());
    } else if (header.datatypecode == 10) {
        size_t pixelCount = width * height;
        size_t currentPixel = 0;
        unsigned char pixel[3];

        while (currentPixel < pixelCount) {
            unsigned char chunkHeader;
            file.read(reinterpret_cast<char*>(&chunkHeader), 1);

            if (chunkHeader < 128) { // Raw packet
                size_t count = chunkHeader + 1;
                for (size_t i = 0; i < count && currentPixel < pixelCount; ++i) {
                    file.read(reinterpret_cast<char*>(pixel), 3);
                    data[currentPixel * 3] = pixel[0];
                    data[currentPixel * 3 + 1] = pixel[1];
                    data[currentPixel * 3 + 2] = pixel[2];
                    currentPixel++;
                }
            } else { // RLE packet
                size_t count = chunkHeader - 127;
                file.read(reinterpret_cast<char*>(pixel), 3);
                for (size_t i = 0; i < count && currentPixel < pixelCount; ++i) {
                    data[currentPixel * 3] = pixel[0];
                    data[currentPixel * 3 + 1] = pixel[1];
                    data[currentPixel * 3 + 2] = pixel[2];
                    currentPixel++;
                }
            }
        }
    }

    // BGR 转 RGB
    for (size_t i = 0; i < data.size(); i += 3) {
        std::swap(data[i], data[i + 2]);
    }
    return true;
}
```

### 3. Model 类集成

```cpp
class Model {
public:
    Texture diffuseTexture;
    
    bool loadDiffuseTexture(const std::string& filename) {
        return diffuseTexture.loadFromTGA(filename);
    }
    
    void renderSolid(Framebuffer& fb, const vec3f& lightDir, const vec3f& eye) {
        // 使用 diffuseTexture 进行渲染...
    }
};
```

### 4. 主程序调整

```cpp
int main() {
    Framebuffer framebuffer(800, 800);
    framebuffer.clear(vec3f(0.5f, 0.5f, 0.5f));
    framebuffer.clearZBuffer();

    Model model;
    if (!model.loadFromObj("resources/obj/african_head.obj")) {
        std::cerr << "Failed to load model" << std::endl;
        return 1;
    }

    if (!model.loadDiffuseTexture("resources/diffuse/african_head_diffuse.tga")) {
        std::cerr << "Failed to load texture" << std::endl;
        return 1;
    }

    model.renderSolid(framebuffer, vec3f(1.0f, 1.0f, 1.0f), vec3f(0.0f, 0.0f, 1.0f));
    framebuffer.flipVertical();
    framebuffer.saveToTGA("output.tga");

    return 0;
}
```

## 技术要点

- TGA 格式支持：扩展 loadTGA 函数，支持 datatypecode == 2（未压缩 RGB）和 datatypecode == 10（RLE 压缩 RGB）。
- RLE 解码：实现 RLE 压缩的解码逻辑，处理 raw 和 RLE 数据包。
- 颜色转换：将 TGA 文件的 BGR 格式转换为 RGB 格式。
- 错误处理：添加详细的调试输出，确保加载失败时能定位问题。

## 验证方法

1. 检查输出图像 output.tga，确认模型表面显示正确的 diffuse 纹理。
2. 验证纹理坐标 (u, v) 的插值是否正确，纹理无拉伸或错位。
3. 确保 RLE 压缩的 TGA 文件能够正常加载并渲染。
4. 检查程序运行时无 "Failed to load texture" 错误输出。

## 修改过程回顾

最初，程序因 "Failed to load texture" 而失败，原因是 african_head_diffuse.tga 文件使用了 RLE 压缩（datatypecode == 10），而原始代码只支持未压缩格式（datatypecode == 2）。通过调试输出确认问题后，我扩展了 loadTGA 函数，添加了对 RLE 压缩的支持，最终成功加载并渲染了 diffuse 材质。
