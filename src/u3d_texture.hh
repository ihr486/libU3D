/*
 * Copyright (C) 2016 Hiroka Ihara
 *
 * This file is part of libU3D.
 *
 * libU3D is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libU3D is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libU3D.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

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
            throw U3D_ERROR << "Textures with more than one continuations are not supported.";
        }
        image_data = NULL;
        uint8_t channels;
        reader >> compression_type >> channels >> attributes;
        if(type != channels) {
            throw U3D_ERROR << "Texture type and channel mask do not match.";
        }
        if(attributes & 0x0001) {
            throw U3D_ERROR << "Texture loading from URI is not currently implemented.";
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
