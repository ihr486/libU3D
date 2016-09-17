#include "texture.hpp"

namespace U3D
{

void Texture::load_continuation(BitStreamReader& reader)
{
    uint32_t image_index = reader.read<uint32_t>();
    if(image_index < continuations.size()) {
        uint32_t bytes_read = reader.read_remainder(continuations[image_index].image_data + continuations[image_index].byte_position, continuations[image_index].byte_count - continuations[image_index].byte_position);
        continuations[image_index].byte_position += bytes_read;
    }
}

}
