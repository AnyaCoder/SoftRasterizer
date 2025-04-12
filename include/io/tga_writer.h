#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

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

bool loadTGA(const std::string& filename, int& width, int& height, std::vector<unsigned char>& data);
