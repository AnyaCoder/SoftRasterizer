#include <fstream>
#include "core/framebuffer.h"

#pragma pack(push, 1)
struct TGAHeader {
    char idlength;
    char colormaptype;
    char datatypecode;
    short colormaporigin;
    short colormaplength;
    char colormapdepth;
    short x_origin;
    short y_origin;
    short width;
    short height;
    char bitsperpixel;
    char imagedescriptor;
};
#pragma pack(pop)

bool Framebuffer::saveToTGA(const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    TGAHeader header = {};
    header.datatypecode = 2; // Uncompressed RGB
    header.width = width;
    header.height = height;
    header.bitsperpixel = 24;
    header.imagedescriptor = 0x20; // Top-left origin

    file.write(reinterpret_cast<char*>(&header), sizeof(header));

    // Write pixel data (BGR format)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const auto& pixel = pixels[y * width + x];
            char color[3] = {
                static_cast<char>(pixel.z * 255),
                static_cast<char>(pixel.y * 255),
                static_cast<char>(pixel.x * 255)
            };
            file.write(color, 3);
        }
    }

    return true;
}
