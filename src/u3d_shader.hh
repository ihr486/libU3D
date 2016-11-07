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

class ShaderGroup;

class Material
{
    friend class ShaderGroup;

    uint32_t attributes;
    static const uint32_t AMBIENT = 1, DIFFUSE = 2, SPECULAR = 4, EMISSIVE = 8, REFLECTIVITY = 16, OPACITY = 32;
    Color3f ambient, diffuse, specular, emissive;
    float reflectivity, opacity;
public:
    Material(BitStreamReader& reader)
    {
        reader >> attributes >> ambient >> diffuse >> specular >> emissive >> reflectivity >> opacity;
    }
    Material()
    {
        attributes = 0x0000003F;
        ambient = Color3f(0.75f, 0.75f, 0.75f);
        diffuse = Color3f(0, 0, 0);
        specular = Color3f(0, 0, 0);
        emissive = Color3f(0, 0, 0);
        reflectivity = 0;
        opacity = 1.0f;
    }
};

struct ShaderGroup
{
    GLuint point_program, spot_program, directional_program, ambient_program;
    struct MaterialParams
    {
        Color3f ambient, diffuse, specular, emissive;
        float reflectivity, opacity;
        void load(GLuint program)
        {
            glUniform4f(glGetUniformLocation(program, "material_diffuse"), diffuse.r, diffuse.g, diffuse.b, 1.0f);
            glUniform4f(glGetUniformLocation(program, "material_specular"), specular.r, specular.g, specular.b, 1.0f);
            glUniform4f(glGetUniformLocation(program, "material_ambient"), ambient.r, ambient.g, ambient.b, 1.0f);
            glUniform4f(glGetUniformLocation(program, "material_emissive"), emissive.r, emissive.g, emissive.r, 1.0f);
            glUniform1f(glGetUniformLocation(program, "material_reflectivity"), reflectivity);

            /*U3D_LOG << "Diffuse = " << diffuse << std::endl;
            U3D_LOG << "Specular = " << specular << std::endl;
            U3D_LOG << "Ambient = " << ambient << std::endl;
            U3D_LOG << "Emissive = " << emissive << std::endl;
            U3D_LOG << "Reflectivity = " << reflectivity << std::endl;*/
        }
        void configure(const Material *material)
        {
            ambient = material->ambient;
            diffuse = material->diffuse;
            specular = material->specular;
            emissive = material->emissive;
            reflectivity = material->reflectivity;
            opacity = material->opacity;
        }
    };
    MaterialParams material;
    uint8_t shader_channels;
    std::string texture_names[8];
    GLuint use(uint8_t type)
    {
        GLuint program = 0;
        switch(type) {
        case 0:
            program = ambient_program;
            break;
        case 1:
            program = directional_program;
            break;
        case 2:
            program = point_program;
            break;
        case 3:
            program = spot_program;
            break;
        }
        static const char *texuniforms[8] = {
            "texture0", "texture1", "texture2", "texture3",
            "texture4", "texture5", "texture6", "texture7"
        };
        if(program != 0) {
            glUseProgram(program);
            material.load(program);
            for(int i = 0; i < 8; i++) {
                if(shader_channels & (1 << i)) {
                    glUniform1i(glGetUniformLocation(program, texuniforms[i]), i);
                }
            }
        }
        return program;
    }
};

class FileStructure;

class LitTextureShader
{
    friend class FileStructure;

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
                U3D_LOG << "Texture channel " << i << std::endl;
                U3D_LOG << "Mode = " << (int)texinfos[i].mode << std::endl;
                U3D_LOG << "Blend = " << (int)texinfos[i].blend_source << std::endl;
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
    ShaderGroup *create_shader_group(const Material* mat);
};

}
