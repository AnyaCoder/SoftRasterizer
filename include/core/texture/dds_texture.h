// include/core/texture/dds_texture.h
#pragma once
#include "core/texture/texture.h"

class DDSTexture : public Texture {
public:
    bool load(const std::string& filename) override;
    vec3f sample(float u, float v) const override;

    bool isCompressed = false;
    std::string compressionFormat; // e.g., "DXT1", "DXT5", "ATI2"

    static bool getCompressionFormat(const std::string& filename, std::string& outFormat, std::string& outError);

private:
#pragma pack(push, 1)
    struct DDSHeader {
        uint32_t size;
        uint32_t flags;
        uint32_t height;
        uint32_t width;
        uint32_t pitchOrLinearSize;
        uint32_t depth;
        uint32_t mipMapCount;
        uint32_t reserved1[11];
        struct {
            uint32_t size;
            uint32_t flags;
            char fourCC[4];
            uint32_t rgbBitCount;
            uint32_t rBitMask;
            uint32_t gBitMask;
            uint32_t bBitMask;
            uint32_t aBitMask;
        } pixelFormat;
        uint32_t caps;
        uint32_t caps2;
        uint32_t caps3;
        uint32_t caps4;
        uint32_t reserved2;
    };
#pragma pack(pop)

    static bool readHeader(std::ifstream& file, DDSHeader& header, std::string& outError);

    bool decompressDXT1(const std::vector<unsigned char>& data, int width, int height);
    bool decompressDXT5(const std::vector<unsigned char>& data, int width, int height);
    bool decompressATI2(const std::vector<unsigned char>& data, int width, int height);
};