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
    if(image_index < continuations.size()) {
        uint32_t bytes_read = reader.read_remainder(continuations[image_index].image_data + continuations[image_index].byte_position, continuations[image_index].byte_count - continuations[image_index].byte_position);
        continuations[image_index].byte_position += bytes_read;
    }
}

}
