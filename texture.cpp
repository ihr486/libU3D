#include "texture.hpp"

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
        throw Error(ERROR_FORMAT) << "Image index " << image_index << "does not exist.";
    }
    uint32_t bytes_read = reader.read_remainder(image_data + byte_position, byte_count - byte_position);
    continuations[image_index].byte_position += bytes_read;
}

Texture::Texture()
{
    height = 8, width = 8, type = RGB, compression_type = RAW;
    image_data = default_texture;
    byte_count = sizeof(default_texture);
}

GLuint Texture::load_texture()
{
    void *image_data = continuations[0].image_data;
    size_t image_size = continuations[0].byte_count;

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    if(compression_type == RAW) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
    } else {
        SDL_Surface *surface = IMG_Load_RW(SDL_RWFromMem(image_data, image_size), true);
        if(surface == NULL) {
            throw Error(ERROR_EXTERNAL, "Failed to load a texture image.");
        }
        switch(surface->format) {
        case SDL_PIXELFORMAT_RGB24:
        case SDL_PIXELFORMAT_RGB888:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
            break;
        case SDL_PIXELFORMAT_BGR24:
        case SDL_PIXELFORMAT_BGR888:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, image_data);
            break;
        case SDL_PIXELFORMAT_RGBA8888:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, image_data);
            break;
        case SDL_PIXELFORMAT_BGRA8888:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, image_data);
            break;
        case SDL_PIXELFORMAT_ARGB8888:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, image_data);
            break;
        case SDL_PIXELFORMAT_ABGR8888:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, image_data);
            break;
        default:
            throw Error(ERROR_EXTERNAL, "Texture image reported to have an unrecognized format.");
        }
        SDL_FreeSurface(surface);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return texture;
}

}
