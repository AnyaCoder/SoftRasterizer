// include/core/texture/dds_texture.h
#pragma once
#include "core/texture/texture.h"

#define DDS_MAGIC 0x20534444 // "DDS "

#define DDSD_CAPS        0x1
#define DDSD_HEIGHT      0x2
#define DDSD_WIDTH       0x4
#define DDSD_PITCH       0x8
#define DDSD_PIXELFORMAT 0x1000
#define DDSD_MIPMAPCOUNT 0x20000
#define DDSD_LINEARSIZE  0x80000
#define DDSD_DEPTH       0x800000

#define DDPF_ALPHAPIXELS 0x1
#define DDPF_ALPHA       0x2 // Deprecated, use DDPF_ALPHAPIXELS instead
#define DDPF_FOURCC      0x4
#define DDPF_RGB         0x40
#define DDPF_YUV         0x200
#define DDPF_LUMINANCE   0x20000

#define DDSCAPS_COMPLEX 0x8
#define DDSCAPS_MIPMAP  0x400000
#define DDSCAPS_TEXTURE 0x1000


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


class DDSTexture : public Texture {
public:
    bool load(const std::string& filename) override;
    vec3f sample(float u, float v, const vec2f& ddx, const vec2f& ddy) const override;

    bool isCompressed = false;
    std::string compressionFormat; // e.g., "DXT1", "DXT5", "ATI2"

    static bool getCompressionFormat(const std::string& filename, std::string& outFormat, std::string& outError);

private:

    static bool readHeader(std::ifstream& file, DDSHeader& header, std::string& outError);

    bool decompressDXT1(const std::vector<unsigned char>& data, int width, int height);
    bool decompressDXT5(const std::vector<unsigned char>& data, int width, int height);
    bool decompressATI2(const std::vector<unsigned char>& data, int width, int height);
};