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

    // Support uncompressed RGB (2), RLE-compressed RGB (10), and RLE-compressed grayscale (11)
    if ((header.datatypecode != 2 && header.datatypecode != 10 && header.datatypecode != 11) ||
        (header.datatypecode == 2 && header.bitsperpixel != 24) ||
        (header.datatypecode == 10 && header.bitsperpixel != 24 && header.bitsperpixel != 32) ||
        (header.datatypecode == 11 && header.bitsperpixel != 8)) { // Updated to check bitsperpixel for datatype 11
        std::cerr << "Unsupported TGA format" << std::endl;
        return false;
    }

    width = header.width;
    height = header.height;

    // Set data size based on datatype
    if (header.datatypecode == 11) {
        data.resize(width * height * 3); // Still allocate RGB for compatibility
    } else {
        data.resize(width * height * 3); // RGB for datatype 2 and 10
    }

    // Skip image ID and color map data
    file.seekg(header.idlength + header.colormaplength * (header.colormapdepth / 8), std::ios::cur);

    // Read pixel data
    if (header.datatypecode == 2) {
        // Uncompressed RGB
        file.read(reinterpret_cast<char*>(data.data()), data.size());
        if (!file.good()) {
            std::cerr << "Failed to read uncompressed pixel data" << std::endl;
            return false;
        }
    } else if (header.datatypecode == 10) {
        // RLE-compressed RGB (24-bit or 32-bit)
        size_t pixelCount = width * height;
        size_t currentPixel = 0;

        if (header.bitsperpixel == 24) {
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
        } else if (header.bitsperpixel == 32) { // New: Handle 32-bit RGBA
            unsigned char pixel[4]; // B, G, R, A
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
                        file.read(reinterpret_cast<char*>(pixel), 4);
                        if (!file.good()) {
                            std::cerr << "Failed to read raw RGBA pixel data" << std::endl;
                            return false;
                        }
                        data[currentPixel * 3] = pixel[0];     // B
                        data[currentPixel * 3 + 1] = pixel[1]; // G
                        data[currentPixel * 3 + 2] = pixel[2]; // R
                        currentPixel++;
                    }
                } else {
                    // RLE packet: next `(chunkHeader - 127)` pixels are the same
                    size_t count = chunkHeader - 127;
                    file.read(reinterpret_cast<char*>(pixel), 4);
                    if (!file.good()) {
                        std::cerr << "Failed to read RLE RGBA pixel data" << std::endl;
                        return false;
                    }
                    for (size_t i = 0; i < count && currentPixel < pixelCount; ++i) {
                        data[currentPixel * 3] = pixel[0];     // B
                        data[currentPixel * 3 + 1] = pixel[1]; // G
                        data[currentPixel * 3 + 2] = pixel[2]; // R
                        currentPixel++;
                    }
                }
            }
        }
    } else if (header.datatypecode == 11) {
        // RLE-compressed grayscale
        size_t pixelCount = width * height;
        size_t currentPixel = 0;
        unsigned char grayValue;

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
                    file.read(reinterpret_cast<char*>(&grayValue), 1);
                    if (!file.good()) {
                        std::cerr << "Failed to read raw grayscale pixel data" << std::endl;
                        return false;
                    }
                    // Convert grayscale to RGB by duplicating the value
                    data[currentPixel * 3] = grayValue;
                    data[currentPixel * 3 + 1] = grayValue;
                    data[currentPixel * 3 + 2] = grayValue;
                    currentPixel++;
                }
            } else {
                // RLE packet: next `(chunkHeader - 127)` pixels are the same
                size_t count = chunkHeader - 127;
                file.read(reinterpret_cast<char*>(&grayValue), 1);
                if (!file.good()) {
                    std::cerr << "Failed to read RLE grayscale pixel data" << std::endl;
                    return false;
                }
                for (size_t i = 0; i < count && currentPixel < pixelCount; ++i) {
                    // Convert grayscale to RGB by duplicating the value
                    data[currentPixel * 3] = grayValue;
                    data[currentPixel * 3 + 1] = grayValue;
                    data[currentPixel * 3 + 2] = grayValue;
                    currentPixel++;
                }
            }
        }
    }

    // Convert BGR to RGB for RGB images only
    if (header.datatypecode != 11) { // Skip for grayscale
        for (size_t i = 0; i < data.size(); i += 3) {
            std::swap(data[i], data[i + 2]);
        }
    }

    return true;
}
