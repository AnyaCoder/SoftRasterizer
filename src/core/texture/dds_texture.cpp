#include "core/texture/dds_texture.h"
#include <fstream>
#include <cstring>
#include <algorithm>

#define DDS_MAGIC 0x20534444 // "DDS "

bool DDSTexture::readHeader(std::ifstream& file, DDSHeader& header, std::string& outError) {
    if (!file.is_open()) {
        outError = "Failed to open file";
        return false;
    }

    uint32_t magic;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    if (magic != DDS_MAGIC) {
        outError = "Invalid DDS magic number";
        return false;
    }

    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (header.size != 124) {
        outError = "Invalid header size";
        return false;
    }

    outError = "";
    return true;
}

bool DDSTexture::load(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    DDSHeader header;
    std::string error;

    if (!readHeader(file, header, error)) {
        compressionFormat = "Unknown";
        return false;
    }

    width = header.width;
    height = header.height;

    bool isDXT1 = false, isDXT5 = false, isATI2 = false;
    if (header.pixelFormat.flags & 0x4) {
        compressionFormat = std::string(header.pixelFormat.fourCC, 4);
        if (strncmp(header.pixelFormat.fourCC, "DXT1", 4) == 0) {
            isDXT1 = true;
            isCompressed = true;
        } else if (strncmp(header.pixelFormat.fourCC, "DXT5", 4) == 0) {
            isDXT5 = true;
            isCompressed = true;
        } else if (strncmp(header.pixelFormat.fourCC, "ATI2", 4) == 0) {
            isATI2 = true;
            isCompressed = true;
        } else {
            return false; // Unsupported compression format
        }
    } else {
        compressionFormat = "Uncompressed";
        return false;
    }

    uint32_t blockSize = isDXT1 ? 8 : 16;
    uint32_t dataSize = ((width + 3) / 4) * ((height + 3) / 4) * blockSize;
    std::vector<unsigned char> compressedData(dataSize);
    file.read(reinterpret_cast<char*>(compressedData.data()), dataSize);

    bool success = false;
    if (isDXT1) {
        success = decompressDXT1(compressedData, width, height);
    } else if (isDXT5) {
        success = decompressDXT5(compressedData, width, height);
    } else if (isATI2) {
        success = decompressATI2(compressedData, width, height);
    }

    return success;
}

bool DDSTexture::getCompressionFormat(const std::string& filename, std::string& outFormat, std::string& outError) {
    std::ifstream file(filename, std::ios::binary);
    DDSHeader header;

    if (!readHeader(file, header, outError)) {
        outFormat = "Unknown";
        return false;
    }

    if (header.pixelFormat.flags & 0x4) {
        outFormat = std::string(header.pixelFormat.fourCC, 4);
    } else {
        outFormat = "Uncompressed";
        outError = "No FourCC (uncompressed format)";
        return false;
    }

    outError = "";
    return true;
}

bool DDSTexture::decompressDXT1(const std::vector<unsigned char>& data, int width, int height) {
    pixels.resize(width * height);

    for (int y = 0; y < height; y += 4) {
        for (int x = 0; x < width; x += 4) {
            size_t offset = ((y / 4) * ((width + 3) / 4) + (x / 4)) * 8;
            if (offset + 8 > data.size()) {
                return false;
            }

            uint16_t c0 = (data[offset] | (data[offset + 1] << 8));
            uint16_t c1 = (data[offset + 2] | (data[offset + 3] << 8));
            uint32_t lookup = (data[offset + 4] | (data[offset + 5] << 8) |
                              (data[offset + 6] << 16) | (data[offset + 7] << 24));

            vec3f colors[4];
            colors[0] = {
                ((c0 >> 11) & 31) / 31.0f,
                ((c0 >> 5) & 63) / 63.0f,
                (c0 & 31) / 31.0f
            };
            colors[1] = {
                ((c1 >> 11) & 31) / 31.0f,
                ((c1 >> 5) & 63) / 63.0f,
                (c1 & 31) / 31.0f
            };
            colors[2] = (c0 > c1) ?
                (colors[0] * 2.0f + colors[1]) / 3.0f :
                (colors[0] + colors[1]) / 2.0f;
            colors[3] = (c0 > c1) ?
                (colors[0] + colors[1] * 2.0f) / 3.0f :
                vec3f(0.0f, 0.0f, 0.0f);

            for (int j = 0; j < 4; ++j) {
                for (int i = 0; i < 4; ++i) {
                    int px = x + i;
                    int py = y + j;
                    if (px < width && py < height) {
                        int index = (lookup >> (2 * (j * 4 + i))) & 0x3;
                        pixels[py * width + px] = colors[index];
                    }
                }
            }
        }
    }
    return true;
}

bool DDSTexture::decompressDXT5(const std::vector<unsigned char>& data, int width, int height) {
    pixels.resize(width * height);

    for (int y = 0; y < height; y += 4) {
        for (int x = 0; x < width; x += 4) {
            size_t offset = ((y / 4) * ((width + 3) / 4) + (x / 4)) * 16;
            if (offset + 16 > data.size()) {
                return false;
            }

            uint8_t alpha0 = data[offset];
            uint8_t alpha1 = data[offset + 1];
            uint64_t alphaBits = 0;
            for (int i = 0; i < 6; ++i) {
                alphaBits |= (uint64_t)data[offset + 2 + i] << (i * 8);
            }

            uint16_t c0 = (data[offset + 8] | (data[offset + 9] << 8));
            uint16_t c1 = (data[offset + 10] | (data[offset + 11] << 8));
            uint32_t lookup = (data[offset + 12] | (data[offset + 13] << 8) |
                              (data[offset + 14] << 16) | (data[offset + 15] << 24));

            vec3f colors[4];
            colors[0] = {
                ((c0 >> 11) & 31) / 31.0f,
                ((c0 >> 5) & 63) / 63.0f,
                (c0 & 31) / 31.0f
            };
            colors[1] = {
                ((c1 >> 11) & 31) / 31.0f,
                ((c1 >> 5) & 63) / 63.0f,
                (c1 & 31) / 31.0f
            };
            colors[2] = (colors[0] * 2.0f + colors[1]) / 3.0f;
            colors[3] = (colors[0] + colors[1] * 2.0f) / 3.0f;

            float alphas[8];
            alphas[0] = alpha0 / 255.0f;
            alphas[1] = alpha1 / 255.0f;
            if (alpha0 > alpha1) {
                for (int i = 0; i < 6; ++i) {
                    alphas[i + 2] = ((6 - i) * alpha0 + (i + 1) * alpha1) / 7.0f / 255.0f;
                }
            } else {
                for (int i = 0; i < 4; ++i) {
                    alphas[i + 2] = ((4 - i) * alpha0 + (i + 1) * alpha1) / 5.0f / 255.0f;
                }
                alphas[6] = 0.0f;
                alphas[7] = 1.0f;
            }

            for (int j = 0; j < 4; ++j) {
                for (int i = 0; i < 4; ++i) {
                    int px = x + i;
                    int py = y + j;
                    if (px < width && py < height) {
                        int colorIndex = (lookup >> (2 * (j * 4 + i))) & 0x3;
                        pixels[py * width + px] = colors[colorIndex];
                    }
                }
            }
        }
    }
    return true;
}

bool DDSTexture::decompressATI2(const std::vector<unsigned char>& data, int width, int height) {
    pixels.resize(width * height);

    for (int y = 0; y < height; y += 4) {
        for (int x = 0; x < width; x += 4) {
            size_t offset = ((y / 4) * ((width + 3) / 4) + (x / 4)) * 16;
            if (offset + 16 > data.size()) {
                return false;
            }

            // ATI2 (BC5) stores two channels (R and G) in two DXT5-like alpha blocks
            // First block: Red channel (X component of normal)
            uint8_t r0 = data[offset];
            uint8_t r1 = data[offset + 1];
            uint64_t rBits = 0;
            for (int i = 0; i < 6; ++i) {
                rBits |= (uint64_t)data[offset + 2 + i] << (i * 8);
            }

            // Second block: Green channel (Y component of normal)
            uint8_t g0 = data[offset + 8];
            uint8_t g1 = data[offset + 9];
            uint64_t gBits = 0;
            for (int i = 0; i < 6; ++i) {
                gBits |= (uint64_t)data[offset + 10 + i] << (i * 8);
            }

            // Decode red channel
            float reds[8];
            reds[0] = r0 / 255.0f;
            reds[1] = r1 / 255.0f;
            if (r0 > r1) {
                for (int i = 0; i < 6; ++i) {
                    reds[i + 2] = ((6 - i) * r0 + (i + 1) * r1) / 7.0f / 255.0f;
                }
            } else {
                for (int i = 0; i < 4; ++i) {
                    reds[i + 2] = ((4 - i) * r0 + (i + 1) * r1) / 5.0f / 255.0f;
                }
                reds[6] = 0.0f;
                reds[7] = 1.0f;
            }

            // Decode green channel
            float greens[8];
            greens[0] = g0 / 255.0f;
            greens[1] = g1 / 255.0f;
            if (g0 > g1) {
                for (int i = 0; i < 6; ++i) {
                    greens[i + 2] = ((6 - i) * g0 + (i + 1) * g1) / 7.0f / 255.0f;
                }
            } else {
                for (int i = 0; i < 4; ++i) {
                    greens[i + 2] = ((4 - i) * g0 + (i + 1) * g1) / 5.0f / 255.0f;
                }
                greens[6] = 0.0f;
                greens[7] = 1.0f;
            }

            // Decode 4x4 block
            for (int j = 0; j < 4; ++j) {
                for (int i = 0; i < 4; ++i) {
                    int px = x + i;
                    int py = y + j;
                    if (px < width && py < height) {
                        int rIndex = (rBits >> (3 * (j * 4 + i))) & 0x7;
                        int gIndex = (gBits >> (3 * (j * 4 + i))) & 0x7;
                        float r = reds[rIndex];
                        float g = greens[gIndex];
                        // Reconstruct blue channel assuming normalized normal
                        float b = std::sqrt(std::max(0.0f, 1.0f - r * r - g * g));
                        pixels[py * width + px] = vec3f(r, g, b);
                    }
                }
            }
        }
    }
    return true;
}

vec3f DDSTexture::sample(float u, float v) const {
    u = u - floorf(u);
    v = v - floorf(v);
    
    int x = static_cast<int>(u * (width - 1));
    int y = static_cast<int>(v * (height - 1));
    
    x = std::max(0, std::min(width - 1, x));
    y = std::max(0, std::min(height - 1, y));
    
    return pixels[y * width + x];
}