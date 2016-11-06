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

#include "u3d_internal.hh"

namespace U3D
{

static const uint8_t default_texture[192] = {
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 102, 51, 255, 102, 51, 255, 102, 51, 255, 102, 51,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 102, 51, 255, 102, 51, 255, 102, 51, 255, 102, 51,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 102, 51, 255, 102, 51, 255, 102, 51, 255, 102, 51,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 102, 51, 255, 102, 51, 255, 102, 51, 255, 102, 51,
    255, 102, 51, 255, 102, 51, 255, 102, 51, 255, 102, 51, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 102, 51, 255, 102, 51, 255, 102, 51, 255, 102, 51, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 102, 51, 255, 102, 51, 255, 102, 51, 255, 102, 51, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 102, 51, 255, 102, 51, 255, 102, 51, 255, 102, 51, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
};

void Texture::load_continuation(BitStreamReader& reader)
{
    uint32_t image_index = reader.read<uint32_t>();
    if(image_index != 0) {
        throw U3D_ERROR << "Image index " << image_index << "does not exist.";
    }
    uint32_t bytes_read = reader.read_remainder(image_data + byte_position, byte_count - byte_position);
    byte_position += bytes_read;
}

Texture::Texture()
{
    height = 8, width = 8, type = RGB, compression_type = RAW;
    image_data = new uint8_t[sizeof(default_texture)];
    memcpy(image_data, default_texture, sizeof(default_texture));
    byte_count = sizeof(default_texture);
}

GLuint Texture::load_texture()
{
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    if(compression_type == RAW) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
    } else {
        SDL_Surface *surface = IMG_Load_RW(SDL_RWFromMem(image_data, byte_count), true);
        if(surface == NULL) {
            throw U3D_ERROR << "Failed to load a texture image.";
        }
        switch(surface->format->format) {
        case SDL_PIXELFORMAT_RGB24:
        case SDL_PIXELFORMAT_RGB888:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, surface->pixels);
            break;
        case SDL_PIXELFORMAT_BGR24:
        case SDL_PIXELFORMAT_BGR888:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, surface->pixels);
            break;
        case SDL_PIXELFORMAT_RGBA8888:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, surface->pixels);
            break;
        case SDL_PIXELFORMAT_BGRA8888:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, surface->pixels);
            break;
        case SDL_PIXELFORMAT_ARGB8888:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, surface->pixels);
            break;
        case SDL_PIXELFORMAT_ABGR8888:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, surface->pixels);
            break;
        default:
            throw U3D_ERROR << "Texture image reported to have an unrecognized format.";
        }
        SDL_FreeSurface(surface);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return texture;
}

}
