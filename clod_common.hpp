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
    struct RenderElement {
        GLuint buffer;
        int count;
        uint32_t flags;
    };
    std::vector<RenderElement> elements;
    GLenum mode;
public:
    static const uint32_t BUFFER_POSITION_MASK = 0x7;
    static const uint32_t BUFFER_NORMAL_MASK = 0x38;
    static const uint32_t BUFFER_DIFFUSE_MASK = 0x3C0;
    static const uint32_t BUFFER_SPECULAR_MASK = 0x3C00;
    static const uint32_t BUFFER_TEXCOORD0_MASK = 0xC000;

    RenderGroup(GLenum mode, int num_elements) : mode(mode) {
        elements.resize(num_elements);
        for(int i = 0; i < num_elements; i++) {
            glGenBuffers(1, &elements[i].buffer);
        }
    }
    ~RenderGroup() {
        for(unsigned int i = 0; i < elements.size(); i++) {
            glDeleteBuffers(1, &elements[i].buffer);
        }
    }
    void load(int index, const GLfloat *src, uint32_t flags, int count) {
        glBindBuffer(GL_ARRAY_BUFFER, elements[index].buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * __builtin_popcount(flags), src, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        elements[index].count = count;
        elements[index].flags = flags;
    }
    void render() {
        for(unsigned int i = 0; i < elements.size(); i++) {
            int stride = sizeof(GLfloat) * __builtin_popcount(elements[i].flags);
            glBindBuffer(GL_ARRAY_BUFFER, elements[i].buffer);
            glEnableVertexAttribArray(0);
            GLfloat *head = 0;
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, head);
            head += 3;
            if(elements[i].flags & BUFFER_NORMAL_MASK) {
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, head);
                head += 3;
            }
            if(elements[i].flags & BUFFER_DIFFUSE_MASK) {
                glEnableVertexAttribArray(2);
                glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, head);
                head += 4;
            }
            if(elements[i].flags & BUFFER_SPECULAR_MASK) {
                glEnableVertexAttribArray(3);
                glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, head);
                head += 4;
            }
            for(int j = 0; j < 8; j++) {
                if(elements[i].flags & (BUFFER_TEXCOORD0_MASK << (2 * j))) {
                    glEnableVertexAttribArray(4 + j);
                    glVertexAttribPointer(4 + j, 2, GL_FLOAT, GL_FALSE, stride, head);
                    head += 2;
                }
            }
            glDrawArrays(mode, 0, elements[i].count);
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
};

class SceneGraph;

class ModelResource
{
    friend class SceneGraph;
    Shading *shading;
public:
    ModelResource() : shading(NULL) {}
    virtual ~ModelResource() {
        if(shading != NULL) {
            delete shading;
        }
    }
    virtual RenderGroup *create_render_group() = 0;
    void add_shading_modifier(Shading *shading)
    {
        this->shading = shading;
    }
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
        uint32_t texcoord_dims[8];
        uint32_t texlayer_count;
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
