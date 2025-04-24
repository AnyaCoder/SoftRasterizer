// src/core/texture/tga_texture.cpp
#include "core/texture/tga_texture.h"
#include "io/tga_writer.h"

namespace { // Anonymous namespace for internal linkage helper functions

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
    
                vec3f sumColor(0.0f, 0.0f, 0.0f);
                // Average 2x2 block (handle boundaries by clamping coordinates)
                // Note: This simple box filter can cause aliasing. More advanced filters exist.
                const vec3f& p00 = inputLevel.pixels[std::min(inputLevel.height - 1, inputY + 0) * inputLevel.width + std::min(inputLevel.width - 1, inputX + 0)];
                const vec3f& p10 = inputLevel.pixels[std::min(inputLevel.height - 1, inputY + 0) * inputLevel.width + std::min(inputLevel.width - 1, inputX + 1)];
                const vec3f& p01 = inputLevel.pixels[std::min(inputLevel.height - 1, inputY + 1) * inputLevel.width + std::min(inputLevel.width - 1, inputX + 0)];
                const vec3f& p11 = inputLevel.pixels[std::min(inputLevel.height - 1, inputY + 1) * inputLevel.width + std::min(inputLevel.width - 1, inputX + 1)];
    
                sumColor = (p00 + p10 + p01 + p11) * 0.25f; // Average the 4 pixels
    
                outputLevel.pixels[y * outputLevel.width + x] = sumColor;
            }
        }
        return true;
    }
    
} // end anonymous namespace


bool TGATexture::load(const std::string& filename) {
    std::vector<unsigned char> raw_data; // Expect raw BGR or BGRA data from loadTGA
    int baseWidth, baseHeight;
    int bytesPerPixel = 0; // We need loadTGA to tell us BPP or infer it

    // Assuming loadTGA exists and populates width, height, raw_data (e.g., BGR format)
    // Modify loadTGA if needed to return BPP or handle different TGA types.
    // For this example, assume loadTGA gives 24-bit BGR.
    if (!loadTGA(filename, baseWidth, baseHeight, raw_data)) { // Pass raw_data by reference
        std::cerr << "Failed to load TGA file: " << filename << std::endl;
        return false;
    }

    bytesPerPixel = 3; // *** Hardcoding 3 BPP (24-bit) - IMPROVE THIS ***

    if (baseWidth <= 0 || baseHeight <= 0 || raw_data.empty()) {
        std::cerr << "TGA file loaded with zero dimensions or no data: " << filename << std::endl;
        return false;
    }

    // Validate data size against dimensions and assumed BPP
    if (raw_data.size() != static_cast<size_t>(baseWidth * baseHeight * bytesPerPixel)) {
        std::cerr << "TGA data size mismatch for " << filename << ". Expected "
                << baseWidth * baseHeight * bytesPerPixel << ", Got " << raw_data.size() << std::endl;
        return false;
    }

    // --- Load Base Level (Level 0) ---
    mipLevels.resize(1);
    mipLevels[0].width = baseWidth;
    mipLevels[0].height = baseHeight;
    mipLevels[0].pixels.resize(baseWidth * baseHeight);

    // Convert raw TGA data to vec3f (RGB float)
    for (int y = 0; y < baseHeight; ++y) {
        for (int x = 0; x < baseWidth; ++x) {
            size_t idx = (y * baseWidth + x) * bytesPerPixel;
            if (bytesPerPixel == 3) { // Assuming 24-bit BGR
                mipLevels[0].pixels[y * baseWidth + x] = vec3f(
                    raw_data[idx + 0] / 255.0f, // R
                    raw_data[idx + 1] / 255.0f, // G
                    raw_data[idx + 2] / 255.0f  // B
                );
            } else if (bytesPerPixel == 4) { // Assuming 32-bit BGRA
                mipLevels[0].pixels[y * baseWidth + x] = vec3f(
                    raw_data[idx + 0] / 255.0f, // R
                    raw_data[idx + 1] / 255.0f, // G
                    raw_data[idx + 2] / 255.0f  // B
                    // Alpha channel raw_data[idx + 3] is ignored here
                );
            }
            // Add handling for other TGA formats (grayscale, indexed etc.) if needed
            else {
                std::cerr << "Unsupported TGA BPP: " << bytesPerPixel << " in " << filename << std::endl;
                mipLevels.clear(); // Clear invalid data
                return false;
            }
        }
    }

    // --- Generate Mipmap Levels ---
    int currentLevelIndex = 0;
    // Limit max levels to prevent infinite loops with tiny textures or excessive memory use
    int maxPossibleLevels = static_cast<int>(std::floor(std::log2(std::max(baseWidth, baseHeight)))) + 1;
    int levelsToGenerate = maxPossibleLevels; // Or set a hard limit like 16

    while (currentLevelIndex < levelsToGenerate - 1) { // Generate up to max levels
        if (mipLevels[currentLevelIndex].width <= 1 && mipLevels[currentLevelIndex].height <= 1) {
            break; // Cannot downsample further
        }

        Texture::MipLevel nextLevel;
        if (!generateNextMipLevel(mipLevels[currentLevelIndex], nextLevel)) {
            std::cerr << "Error generating mip level " << (currentLevelIndex + 1) << " for " << filename << std::endl;
            break; // Stop generating if error occurs
        }

        // Check if dimensions became invalid during generation
        if (nextLevel.width <= 0 || nextLevel.height <= 0 || nextLevel.pixels.empty()) {
            std::cerr << "Warning: Mip level " << (currentLevelIndex + 1) << " generated invalid dimensions/pixels for " << filename << ". Stopping." << std::endl;
            break;
        }


        mipLevels.push_back(std::move(nextLevel));
        currentLevelIndex++;
    }
    std::cout << "Generated: " << currentLevelIndex << " MipLevels" << std::endl;
    return !mipLevels.empty() && !mipLevels[0].pixels.empty(); // Success if base level is valid
}

// Accurate sampling function using derivatives
vec3f TGATexture::sample(float u, float v, const vec2f& ddx, const vec2f& ddy) const {
     // This implementation is identical to DDSTexture::sample
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
    if (rho_sq > 1e-9f) { // Use a small epsilon
        lod = 0.5f * std::log2(rho_sq);
    }
    lod = std::max(0.0f, lod); // Clamp LOD >= 0

    // Determine levels and interpolation factor
    int maxLevel = static_cast<int>(mipLevels.size()) - 1;
    int level0_idx = static_cast<int>(std::floor(lod));
    level0_idx = std::min(level0_idx, maxLevel);

    vec3f color0 = sampleBilinear(mipLevels[level0_idx], u, v);

    if (level0_idx == maxLevel) {
        return color0;
    }

    int level1_idx = level0_idx + 1;
    vec3f color1 = sampleBilinear(mipLevels[level1_idx], u, v);

    float level_t = lod - static_cast<float>(level0_idx);

    // Trilinear interpolation
    return color0 * (1.0f - level_t) + color1 * level_t;
}