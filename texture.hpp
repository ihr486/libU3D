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
    struct Continuation
    {
        uint8_t compression_type, channels;
        uint16_t attributes;
        uint32_t byte_count, byte_position;
        uint8_t *image_data;
    };
    std::vector<Continuation> continuations;
public:
    Texture(BitStreamReader& reader)
    {
        reader >> height >> width >> type;
        uint32_t continuation_count = reader.read<uint32_t>();
        continuations.resize(continuation_count);
        for(unsigned int i = 0; i < continuation_count; i++) {
            continuations[i].image_data = NULL;
            reader >> continuations[i].compression_type >> continuations[i].channels >> continuations[i].attributes;
            if(continuations[i].attributes & 0x0001) {
                std::fprintf(stderr, "External texture reference is not implemented in the current version.\n");
                return;
            } else {
                reader >> continuations[i].byte_count;
                continuations[i].image_data = new uint8_t[continuations[i].byte_count];
            }
            continuations[i].byte_position = 0;
        }
    }
    ~Texture()
    {
        for(unsigned int i = 0; i < continuations.size(); i++) {
            if(continuations[i].image_data != NULL) {
                delete[] continuations[i].image_data;
            }
        }
    }
    void load_continuation(BitStreamReader& reader);
};

}
