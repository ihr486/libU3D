#pragma once

#include <stdint.h>
#include <cstdio>
#include <vector>

#include "bitstream.hpp"

namespace U3D
{

class Texture
{
    uint32_t width, height;
    uint8_t type;
    static const uint8_t ALPHA = 1, BLUE = 2, GREEN = 4, RED = 8, RGB = 14, RGBA = 15, LUMINANCE = 16;
    uint8_t compression_type;
    static const uint8_t RAW = 0, JPEG24 = 1, PNG = 2, JPEG8 = 3, TIFF = 4;
    uint16_t attributes;
    uint32_t byte_count, byte_position;
    uint8_t *image_data;
    std::string uri;
public:
    Texture(BitStreamReader& reader)
    {
        image_data = NULL;
        reader >> height >> width >> type;
        uint32_t continuation_count = reader.read<uint32_t>();
        if(continuation_count != 1) {
            throw Error(ERROR_DEVIATION, "Textures with more than one continuations are not supported.");
        }
        image_data = NULL;
        uint8_t channels;
        reader >> compression_type >> channels >> attributes;
        if(type != channels) {
            throw Error(ERROR_FORMAT, "Texture type and channel mask do not match.");
        }
        if(attributes & 0x0001) {
            throw Error(ERROR_TODO, "Texture loading from URI is not currently implemented.");
        }
        reader >> byte_count;
        byte_position = 0;
        image_data = new uint8_t[byte_count];
    }
    Texture();
    ~Texture()
    {
        if(image_data != NULL) delete[] image_data;
    }
    GLuint load_texture();
    void load_continuation(BitStreamReader& reader);
};

}
