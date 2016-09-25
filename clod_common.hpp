#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

#include <GL/glew.h>

#include "types.hpp"

namespace U3D
{

class Shading
{
    uint32_t chain_index, attributes;
    static const uint32_t SHADING_MESH = 1, SHADING_LINE = 2, SHADING_POINT = 4, SHADING_GLYPH = 8;
    std::vector<std::vector<std::string> > shader_names;
public:
    Shading(BitStreamReader& reader)
    {
        reader >> chain_index >> attributes;
        uint32_t list_count = reader.read<uint32_t>();
        shader_names.resize(list_count);
        for(unsigned int i = 0; i < list_count; i++) {
            uint32_t shader_count = reader.read<uint32_t>();
            if(shader_count != 1) {
                std::cerr << "Shaders with shader count greater than 1 are ignored." << std::endl;
            }
            shader_names[i].resize(shader_count);
            for(unsigned int j = 0; j < shader_count; j++) {
                reader >> shader_names[i][j];
            }
        }
    }
};

class RenderGroup
{
    std::vector<GLuint> vertex_buffers;
public:
    RenderGroup(int n);
    ~RenderGroup();
    void render();
};

class ModelResource
{
public:
    ModelResource() {}
    virtual ~ModelResource() {}
    virtual RenderGroup *create_render_group() = 0;
};

class CLOD_Object
{
protected:
    //Maximum mesh description
    uint32_t attributes;
    static const uint32_t EXCLUDE_NORMALS = 1;
    uint32_t face_count, position_count, normal_count;
    uint32_t diffuse_count, specular_count, texcoord_count;
    struct ShadingDesc {
        uint32_t attributes;
        std::vector<uint32_t> texcoord_dims;
    };
    static const uint32_t VERTEX_DIFFUSE_COLOR = 1, VERTEX_SPECULAR_COLOR = 2;
    std::vector<ShadingDesc> shading_descs;
    //CLoD description
    uint32_t min_res, max_res;
    //Quality factors
    uint32_t position_quality, normal_quality, texcoord_quality;
    //Inverse quantization parameters
    float position_iq, normal_iq, texcoord_iq, diffuse_iq, specular_iq;
    //Resource parameters
    float normal_crease, normal_update, normal_tolerance;
    //Skeleton description
    struct Bone {
        std::string name, parent_name;
        uint32_t attributes;
        float length;
        Vector3f displacement;
        Quaternion4f orientation;
        uint32_t link_count;
        float link_length;
        Vector2f start_joint_center, start_joint_scale;
        Vector2f end_joint_center, end_joint_scale;
    };
    std::vector<Bone> skeleton;
    std::vector<Vector3f> positions, normals;
    std::vector<Color4f> diffuse_colors, specular_colors;
    std::vector<TexCoord4f> texcoords;
public:
    CLOD_Object(bool clod_desc_flag, BitStreamReader& reader);
    CLOD_Object() : face_count(0), position_count(0), normal_count(0), diffuse_count(0), specular_count(0), texcoord_count(0) , min_res(0), max_res(0) {}
protected:
    uint32_t get_texlayer_count(uint32_t shading_id) const
    {
        return shading_descs[shading_id].texcoord_dims.size();
    }
};

template<typename T> static inline void insert_unique(std::vector<T>& cont, T val)
{
    typename std::vector<T>::iterator i;
    for(i = cont.begin(); i != cont.end(); i++) {
        if(*i == val) return;
        else if(*i < val) {
            cont.insert(i, val);
            return;
        }
    }
    if(i == cont.end()) {
        cont.push_back(val);
    }
}

template<typename T> static inline void greater_unique_sort(std::vector<T>& cont)
{
    std::sort(cont.begin(), cont.end(), std::greater<T>());
    cont.erase(std::unique(cont.begin(), cont.end()), cont.end());
}

template<typename T> static inline void print_vector(const std::vector<T>& v, const std::string& name)
{
    std::cerr << name << " : ";
    for(typename std::vector<T>::const_iterator i = v.begin(); i != v.end(); i++) {
        std::cerr << *i << " ";
    }
    std::cerr << std::endl;
}

}
