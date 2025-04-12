// src/io/tga_writer.cpp
#include "core/framebuffer.h"
#include "io/tga_writer.h"


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
    
    file.close();
    return !file.fail();
}

bool loadTGA(const std::string& filename, int& width, int& height, std::vector<unsigned char>& data) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Cannot open file: " << filename << std::endl;
        return false;
    }

    TGAHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    std::cerr << "Filename: " << filename << " , datatypecode: " << (int)header.datatypecode << ", bitsperpixel: " << (int)header.bitsperpixel << std::endl;

    // Support both uncompressed (2) and RLE compressed (10) RGB images
    if ((header.datatypecode != 2 && header.datatypecode != 10) || header.bitsperpixel != 24) {
        std::cerr << "Unsupported TGA format" << std::endl;
        return false;
    }

    width = header.width;
    height = header.height;
    data.resize(width * height * 3);

    // Skip image ID and color map data
    file.seekg(header.idlength + header.colormaplength * (header.colormapdepth / 8), std::ios::cur);

    // Read pixel data (BGR format)
    if (header.datatypecode == 2) {
        // Uncompressed RGB
        file.read(reinterpret_cast<char*>(data.data()), data.size());
        if (!file.good()) {
            std::cerr << "Failed to read uncompressed pixel data" << std::endl;
            return false;
        }
    } else if (header.datatypecode == 10) {
        // RLE compressed RGB
        size_t pixelCount = width * height;
        size_t currentPixel = 0;
        unsigned char pixel[3];

        while (currentPixel < pixelCount) {
            unsigned char chunkHeader;
            file.read(reinterpret_cast<char*>(&chunkHeader), 1);
            if (!file.good()) {
                std::cerr << "Failed to read RLE chunk header" << std::endl;
                return false;
            }

            if (chunkHeader < 128) {
                // Raw packet: next `chunkHeader + 1` pixels are uncompressed
                size_t count = chunkHeader + 1;
                for (size_t i = 0; i < count && currentPixel < pixelCount; ++i) {
                    file.read(reinterpret_cast<char*>(pixel), 3);
                    if (!file.good()) {
                        std::cerr << "Failed to read raw pixel data" << std::endl;
                        return false;
                    }
                    data[currentPixel * 3] = pixel[0];
                    data[currentPixel * 3 + 1] = pixel[1];
                    data[currentPixel * 3 + 2] = pixel[2];
                    currentPixel++;
                }
            } else {
                // RLE packet: next `(chunkHeader - 127)` pixels are the same
                size_t count = chunkHeader - 127;
                file.read(reinterpret_cast<char*>(pixel), 3);
                if (!file.good()) {
                    std::cerr << "Failed to read RLE pixel data" << std::endl;
                    return false;
                }
                for (size_t i = 0; i < count && currentPixel < pixelCount; ++i) {
                    data[currentPixel * 3] = pixel[0];
                    data[currentPixel * 3 + 1] = pixel[1];
                    data[currentPixel * 3 + 2] = pixel[2];
                    currentPixel++;
                }
            }
        }
    }

    // Convert BGR to RGB
    for (size_t i = 0; i < data.size(); i += 3) {
        std::swap(data[i], data[i + 2]);
    }

    return true;
}
