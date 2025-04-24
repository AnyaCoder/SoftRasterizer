#include "core/texture/dds_texture.h"
#include <fstream>
#include <cstring>
#include <algorithm>

namespace { // Anonymous namespace for internal linkage helper functions

    // --- DDS Decompression Block Decoders ---
    void decodeDXT1Block(const unsigned char* block, vec3f colors[4], uint32_t& lookup) {
        uint16_t c0_16 = block[0] | (block[1] << 8);
        uint16_t c1_16 = block[2] | (block[3] << 8);
        lookup = block[4] | (block[5] << 8) | (block[6] << 16) | (block[7] << 24);
    
        colors[0] = { ((c0_16 >> 11) & 31) / 31.0f, ((c0_16 >> 5) & 63) / 63.0f, (c0_16 & 31) / 31.0f };
        colors[1] = { ((c1_16 >> 11) & 31) / 31.0f, ((c1_16 >> 5) & 63) / 63.0f, (c1_16 & 31) / 31.0f };
    
        if (c0_16 > c1_16) {
            colors[2] = (colors[0] * 2.0f + colors[1]) / 3.0f;
            colors[3] = (colors[0] + colors[1] * 2.0f) / 3.0f;
        } else {
            colors[2] = (colors[0] + colors[1]) / 2.0f;
            colors[3] = vec3f(0.0f, 0.0f, 0.0f); // Transparent black
        }
    }
    
    void decodeDXT5Block(const unsigned char* block, vec3f colors[4], float alphas[8], uint32_t& colorLookup, uint64_t& alphaLookup) {
        alphas[0] = block[0] / 255.0f;
        alphas[1] = block[1] / 255.0f;
        alphaLookup = 0;
        for (int i = 0; i < 6; ++i) alphaLookup |= (uint64_t)block[2 + i] << (i * 8);
    
        if (block[0] > block[1]) {
            for (int i = 0; i < 6; ++i) alphas[i + 2] = ((6 - i) * block[0] + (i + 1) * block[1]) / 7.0f / 255.0f;
        } else {
            for (int i = 0; i < 4; ++i) alphas[i + 2] = ((4 - i) * block[0] + (i + 1) * block[1]) / 5.0f / 255.0f;
            alphas[6] = 0.0f; alphas[7] = 1.0f;
        }
        decodeDXT1Block(block + 8, colors, colorLookup); // Color part is DXT1
    }
    
    void decodeATI2Block(const unsigned char* block, float reds[8], float greens[8], uint64_t& rLookup, uint64_t& gLookup) {
        // Red channel block (first 8 bytes) - DXT5 alpha style
        reds[0] = block[0] / 255.0f; reds[1] = block[1] / 255.0f; rLookup = 0;
        for (int i = 0; i < 6; ++i) rLookup |= (uint64_t)block[2 + i] << (i * 8);
        if (block[0] > block[1]) { for (int i = 0; i < 6; ++i) reds[i + 2] = ((6 - i) * block[0] + (i + 1) * block[1]) / 7.0f / 255.0f; }
        else { for (int i = 0; i < 4; ++i) reds[i + 2] = ((4 - i) * block[0] + (i + 1) * block[1]) / 5.0f / 255.0f; reds[6] = 0.0f; reds[7] = 1.0f; }
    
        // Green channel block (next 8 bytes) - DXT5 alpha style
        greens[0] = block[8] / 255.0f; greens[1] = block[9] / 255.0f; gLookup = 0;
        for (int i = 0; i < 6; ++i) gLookup |= (uint64_t)block[10 + i] << (i * 8);
        if (block[8] > block[9]) { for (int i = 0; i < 6; ++i) greens[i + 2] = ((6 - i) * block[8] + (i + 1) * block[9]) / 7.0f / 255.0f; }
        else { for (int i = 0; i < 4; ++i) greens[i + 2] = ((4 - i) * block[8] + (i + 1) * block[9]) / 5.0f / 255.0f; greens[6] = 0.0f; greens[7] = 1.0f; }
    }
    
    // --- Per-Level Decompressors ---
    
    bool decompressDXT1LevelInternal(const std::vector<unsigned char>& data, int levelWidth, int levelHeight, std::vector<vec3f>& outPixels) {
        outPixels.resize(levelWidth * levelHeight);
        uint32_t blocksWide = (levelWidth + 3) / 4;
        uint32_t blocksHigh = (levelHeight + 3) / 4;
        size_t expectedDataSize = blocksWide * blocksHigh * 8;
        if (data.size() < expectedDataSize) {
             std::cerr << "DXT1 data size mismatch for level " << levelWidth << "x" << levelHeight << ". Expected " << expectedDataSize << ", Got " << data.size() << std::endl;
             return false;
        }
    
        size_t blockOffset = 0;
        for (uint32_t y = 0; y < blocksHigh; ++y) {
            for (uint32_t x = 0; x < blocksWide; ++x) {
                const unsigned char* block = data.data() + blockOffset;
                vec3f colors[4];
                uint32_t lookup;
                decodeDXT1Block(block, colors, lookup);

                for (int j = 0; j < 4; ++j) { // Pixel y within block
                    int py = y * 4 + j;
                    if (py >= levelHeight) continue;
                    for (int i = 0; i < 4; ++i) { // Pixel x within block
                        int px = x * 4 + i;
                        if (px >= levelWidth) continue;
                        int index = (lookup >> (2 * (j * 4 + i))) & 0x3;
                        outPixels[py * levelWidth + px] = colors[index];
                    }
                }
                blockOffset += 8;
            }
        }
        return true;
    }
    
    bool decompressDXT5LevelInternal(const std::vector<unsigned char>& data, int levelWidth, int levelHeight, std::vector<vec3f>& outPixels) {
        outPixels.resize(levelWidth * levelHeight);
        uint32_t blocksWide = (levelWidth + 3) / 4;
        uint32_t blocksHigh = (levelHeight + 3) / 4;
        size_t expectedDataSize = blocksWide * blocksHigh * 16;
        if (data.size() < expectedDataSize) {
            std::cerr << "DXT5 data size mismatch for level " << levelWidth << "x" << levelHeight << ". Expected " << expectedDataSize << ", Got " << data.size() << std::endl;
            return false;
        }
    
        size_t blockOffset = 0;
        for (uint32_t y = 0; y < blocksHigh; ++y) {
            for (uint32_t x = 0; x < blocksWide; ++x) {
                const unsigned char* block = data.data() + blockOffset;
                vec3f colors[4];
                float alphas[8];
                uint32_t colorLookup;
                uint64_t alphaLookup;
                decodeDXT5Block(block, colors, alphas, colorLookup, alphaLookup);
    
                for (int j = 0; j < 4; ++j) {
                    int py = y * 4 + j;
                    if (py >= levelHeight) continue;
                    for (int i = 0; i < 4; ++i) {
                        int px = x * 4 + i;
                        if (px >= levelWidth) continue;
                        int colorIndex = (colorLookup >> (2 * (j * 4 + i))) & 0x3;
                        // int alphaIndex = (alphaLookup >> (3 * (j * 4 + i))) & 0x7; // If needed
                        outPixels[py * levelWidth + px] = colors[colorIndex]; // Store only RGB for now
                    }
                }
                 blockOffset += 16;
            }
        }
        return true;
    }
    
    bool decompressATI2LevelInternal(const std::vector<unsigned char>& data, int levelWidth, int levelHeight, std::vector<vec3f>& outPixels) {
        outPixels.resize(levelWidth * levelHeight);
        uint32_t blocksWide = (levelWidth + 3) / 4;
        uint32_t blocksHigh = (levelHeight + 3) / 4;
        size_t expectedDataSize = blocksWide * blocksHigh * 16;
        if (data.size() < expectedDataSize) {
             std::cerr << "ATI2 data size mismatch for level " << levelWidth << "x" << levelHeight << ". Expected " << expectedDataSize << ", Got " << data.size() << std::endl;
             return false;
        }
    
        size_t blockOffset = 0;
        for (uint32_t y = 0; y < blocksHigh; ++y) {
            for (uint32_t x = 0; x < blocksWide; ++x) {
                const unsigned char* block = data.data() + blockOffset;
                float reds[8], greens[8];
                uint64_t rLookup, gLookup;
                decodeATI2Block(block, reds, greens, rLookup, gLookup);

                for (int j = 0; j < 4; ++j) {
                    int py = y * 4 + j;
                    if (py >= levelHeight) continue;
                    for (int i = 0; i < 4; ++i) {
                        int px = x * 4 + i;
                        if (px >= levelWidth) continue;
                        int rIndex = (rLookup >> (3 * (j * 4 + i))) & 0x7;
                        int gIndex = (gLookup >> (3 * (j * 4 + i))) & 0x7;
                        float r = reds[rIndex];
                        float g = greens[gIndex];
                        float b = std::sqrt(std::max(0.0f, 1.0f - r * r - g * g)); // Reconstruct B
                        outPixels[py * levelWidth + px] = vec3f(r, g, b);
                    }
                }
                blockOffset += 16;
            }
        }
        return true;
    }
    
} // end anonymous namespace
    
bool DDSTexture::readHeader(std::ifstream& file, DDSHeader& header, std::string& outError) {
    if (!file.is_open() || !file.good()) {
        outError = "File stream is not open or in a bad state.";
        return false;
    }

    uint32_t magic;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    if (!file || magic != DDS_MAGIC) {
        outError = "Invalid DDS magic number or read error.";
        file.seekg(0, std::ios::beg); // Rewind if magic number read failed? Optional.
        return false;
    }

    file.read(reinterpret_cast<char*>(&header), sizeof(DDSHeader));
    if (!file || header.size != 124) {
        outError = "Invalid DDS header size or read error.";
        return false;
    }

    // Basic validation of flags
    if (!((header.flags & DDSD_CAPS) && (header.flags & DDSD_HEIGHT) &&
          (header.flags & DDSD_WIDTH) && (header.flags & DDSD_PIXELFORMAT))) {
        outError = "Required DDS header flags missing.";
        return false;
    }

    if (!(header.caps & DDSCAPS_TEXTURE)) {
        outError = "DDS file is missing DDSCAPS_TEXTURE flag.";
        return false;
    }


    // Optional: Check for DX10 header if FourCC is "DX10"
    // This example does not handle the DX10 extension.

    outError = "";
    return true;
}

bool DDSTexture::decompressDXT1(const std::vector<unsigned char>& data, int w, int h) {
    if(mipLevels.empty()) mipLevels.resize(1); // Ensure base level exists
    mipLevels[0].width = w;
    mipLevels[0].height = h;
    return decompressDXT1LevelInternal(data, w, h, mipLevels[0].pixels);
}

bool DDSTexture::decompressDXT5(const std::vector<unsigned char>& data, int w, int h) {
    if(mipLevels.empty()) mipLevels.resize(1);
    mipLevels[0].width = w;
    mipLevels[0].height = h;
    return decompressDXT5LevelInternal(data, w, h, mipLevels[0].pixels);
}

bool DDSTexture::decompressATI2(const std::vector<unsigned char>& data, int w, int h) {
    if(mipLevels.empty()) mipLevels.resize(1);
    mipLevels[0].width = w;
    mipLevels[0].height = h;
    return decompressATI2LevelInternal(data, w, h, mipLevels[0].pixels);
}


bool DDSTexture::load(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Failed to open DDS file: " << filename << std::endl;
        return false;
    }

    DDSHeader header;
    std::string error;
    if (!readHeader(file, header, error)) {
        std::cerr << "Error reading DDS header for " << filename << ": " << error << std::endl;
        return false;
    }

    int baseWidth = header.width;
    int baseHeight = header.height;

    // Determine compression type and block size
    bool isDXT1 = false, isDXT5 = false, isATI2 = false;
    uint32_t blockSize = 0; // Bytes per 4x4 block

    if (header.pixelFormat.flags & DDPF_FOURCC) {
        // Check for DX10 header if necessary
        if (strncmp(header.pixelFormat.fourCC, "DX10", 4) == 0) {
            std::cerr << "Error: DX10 DDS header extension not supported in " << filename << std::endl;
            return false; // Add DX10 support later if needed
        }
        // Standard FourCC formats
        else if (strncmp(header.pixelFormat.fourCC, "DXT1", 4) == 0) { isDXT1 = true; blockSize = 8; compressionFormat = "DXT1"; }
        else if (strncmp(header.pixelFormat.fourCC, "DXT5", 4) == 0) { isDXT5 = true; blockSize = 16; compressionFormat = "DXT5"; }
        else if (strncmp(header.pixelFormat.fourCC, "ATI2", 4) == 0 || strncmp(header.pixelFormat.fourCC, "BC5U", 4) == 0) { isATI2 = true; blockSize = 16; compressionFormat = "ATI2/BC5"; }
        // Add other FourCC checks (DXT2/3/4, BC4, etc.) if needed
        else {
            std::cerr << "Unsupported DDS FourCC format: " << std::string(header.pixelFormat.fourCC, 4) << " in " << filename << std::endl;
            return false;
        }
        isCompressed = true;
    } else {
        // Handle uncompressed formats if needed (checking rgbBitCount, masks)
        std::cerr << "Error: Uncompressed DDS formats not supported in " << filename << std::endl;
        compressionFormat = "Uncompressed";
        isCompressed = false;
        return false;
    }

    // Determine number of mip levels
    uint32_t numLevels = 1;
    if ((header.flags & DDSD_MIPMAPCOUNT) && (header.caps & DDSCAPS_MIPMAP)) {
        numLevels = header.mipMapCount;
    }

    if (numLevels == 0) { // Header might have 0 mipmaps if only base level exists
        numLevels = 1;
    }


    mipLevels.resize(numLevels);

    // Load each mip level
    int currentWidth = baseWidth;
    int currentHeight = baseHeight;

    for (uint32_t level = 0; level < numLevels; ++level) {
        if (currentWidth <= 0 || currentHeight <= 0) {
             std::cerr << "Warning: Invalid dimensions (" << currentWidth << "x" << currentHeight
                       << ") calculated for mip level " << level << " in " << filename << ". Stopping load." << std::endl;
             mipLevels.resize(level); // Keep loaded levels up to this point
             break;
        }

        mipLevels[level].width = currentWidth;
        mipLevels[level].height = currentHeight;

        uint32_t levelWidthBlocks = (currentWidth + 3) / 4;
        uint32_t levelHeightBlocks = (currentHeight + 3) / 4;
        uint32_t dataSize = levelWidthBlocks * levelHeightBlocks * blockSize;

        if (dataSize == 0) {
             std::cerr << "Warning: Zero data size calculated for mip level " << level << " in " << filename << ". Stopping load." << std::endl;
             mipLevels.resize(level);
             break;
        }

        std::vector<unsigned char> compressedData(dataSize);
        file.read(reinterpret_cast<char*>(compressedData.data()), dataSize);

        if (!file) {
            std::cerr << "Error reading data for mip level " << level << " (size " << dataSize << ") in " << filename << ". Read " << file.gcount() << " bytes."<< std::endl;
            mipLevels.clear();
            return false;
        }

        // Decompress this level using internal helper functions
        bool success = false;
        if (isDXT1) success = decompressDXT1LevelInternal(compressedData, currentWidth, currentHeight, mipLevels[level].pixels);
        else if (isDXT5) success = decompressDXT5LevelInternal(compressedData, currentWidth, currentHeight, mipLevels[level].pixels);
        else if (isATI2) success = decompressATI2LevelInternal(compressedData, currentWidth, currentHeight, mipLevels[level].pixels);

        if (!success) {
            std::cerr << "Error decompressing mip level " << level << " in " << filename << std::endl;
            mipLevels.clear();
            return false;
        }

        // Calculate dimensions for the next level
        currentWidth = std::max(1, currentWidth / 2);
        currentHeight = std::max(1, currentHeight / 2);
    }

    // Check if we actually loaded any levels
    if (mipLevels.empty() || mipLevels[0].pixels.empty()) {
        std::cerr << "Error: No valid mip levels loaded for " << filename << std::endl;
        return false;
    }

    return true; // Successfully loaded
}

// Static function to get format without loading data
bool DDSTexture::getCompressionFormat(const std::string& filename, std::string& outFormat, std::string& outError) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        outError = "Failed to open file: " + filename;
        outFormat = "Unknown";
        return false;
    }

    DDSHeader header;
    if (!readHeader(file, header, outError)) { // Use static readHeader
        outFormat = "Unknown";
        return false; // Error already set by readHeader
    }

    if (header.pixelFormat.flags & DDPF_FOURCC) {
        outFormat = std::string(header.pixelFormat.fourCC, 4);
        // Handle DX10 case if needed: check if outFormat == "DX10", read DX10 header, determine specific format
    } else if (header.pixelFormat.flags & DDPF_RGB) {
         outFormat = "Uncompressed RGB"; // Could add bit depth info
    } else if (header.pixelFormat.flags & DDPF_LUMINANCE) {
         outFormat = "Uncompressed Luminance"; // Could add bit depth/alpha info
    } else if (header.pixelFormat.flags & DDPF_ALPHA) {
         outFormat = "Uncompressed Alpha";
    } else {
        outFormat = "Uncompressed Unknown";
        outError = "No recognized flags for uncompressed format.";
    }

    outError = "";
    return true;
}


// Accurate sampling function using derivatives
vec3f DDSTexture::sample(float u, float v, const vec2f& ddx, const vec2f& ddy) const {
    if (mipLevels.empty() || mipLevels[0].pixels.empty()) {
        return vec3f(1.0f, 0.0f, 1.0f); // Magenta error color
    }

    const auto& baseLevel = mipLevels[0];
    float baseWidth = static_cast<float>(baseLevel.width);
    float baseHeight = static_cast<float>(baseLevel.height);

    // Calculate rho squared
    float rho_sq = std::max(ddx.lengthSq() * baseWidth * baseWidth,
                            ddy.lengthSq() * baseHeight * baseHeight);

    // Calculate LOD level
    float lod = 0.0f;
    if (rho_sq > 1e-9f) { // Use a small epsilon to avoid log(0)
        lod = 0.5f * std::log2(rho_sq);
    }
    
    lod = std::max(0.0f, lod); // Clamp LOD >= 0

    // Determine levels and interpolation factor
    int maxLevel = static_cast<int>(mipLevels.size()) - 1;
    int level0_idx = static_cast<int>(std::floor(lod));
    level0_idx = std::min(level0_idx, maxLevel);

    vec3f color0 = sampleBilinear(mipLevels[level0_idx], u, v);

    if (level0_idx == maxLevel) {
        return color0; // Only one level needed
    }

    int level1_idx = level0_idx + 1;
    vec3f color1 = sampleBilinear(mipLevels[level1_idx], u, v);

    float level_t = lod - static_cast<float>(level0_idx);

    // Trilinear interpolation
    return color0 * (1.0f - level_t) + color1 * level_t;
}