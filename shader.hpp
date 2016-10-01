#pragma once

#include <stdint.h>
#include <string>

#include <GL/glew.h>

#include "bitstream.hpp"

namespace U3D
{

class LitTextureShader
{
    uint32_t attributes;
    static const uint32_t LIGHTING_ENABLED = 1, ALPHA_TEST_ENABLED = 2, USE_VERTEX_COLOR = 4;
    float alpha_reference;
    uint32_t alpha_function, blend_function;
    static const uint32_t NEVER = 0x610, LESS = 0x611, GREATER = 0x612, EQUAL = 0x613, NOT_EQUAL = 0x614, LEQUAL = 0x615, GEQUAL = 0x616, ALWAYS = 0x617;
    static const uint32_t FB_ADD = 0x604, FB_MULTIPLY = 0x605, FB_ALPHA_BLEND = 0x606, FB_INV_ALPHA_BLEND = 0x607;
    uint32_t render_pass_flags;
    uint32_t shader_channels, alpha_texture_channels;
    std::string material_name;
    struct TextureInfo
    {
        std::string name;
        float intensity;
        uint8_t blend_function;
        uint8_t blend_source;
        float blend_constant;
        uint8_t mode;
        Matrix4f transform;
        Matrix4f wrap_transform;
        uint8_t repeat;
    };
    static const uint8_t MULTIPLY = 0, ADD = 1, REPLACE = 2, BLEND = 3;
    static const uint8_t PIXEL_ALPHA = 0, CONSTANT = 1;
    static const uint8_t TM_NONE = 0, TM_PLANAR = 1, TM_CYLINDRICAL = 2, TM_SPHERICAL = 3, TM_REFLECTION = 4;
    static const uint8_t REPEAT_FIRST = 1, REPEAT_SECOND = 2;
    TextureInfo texinfos[8];
public:
    LitTextureShader(BitStreamReader& reader)
    {
        reader >> attributes >> alpha_reference >> alpha_function >> blend_function;
        reader >> render_pass_flags >> shader_channels >> alpha_texture_channels >> material_name;
        for(unsigned int i = 0; i < 8; i++) {
            if(shader_channels & (1 << i)) {
                reader >> texinfos[i].name >> texinfos[i].intensity >> texinfos[i].blend_function >> texinfos[i].blend_source;
                reader >> texinfos[i].blend_constant >> texinfos[i].mode >> texinfos[i].transform >> texinfos[i].wrap_transform;
                reader >> texinfos[i].repeat;
            }
        }
    }
    LitTextureShader()
    {
        attributes = 0x00000000;
        alpha_reference = 0;
        alpha_function = 0x00000617;
        blend_function = 0x00000606;
        render_pass_flags = 0x00000001;
        material_name = "";
        shader_channels = 0;
        alpha_texture_channels = 0;
    }
};

}
