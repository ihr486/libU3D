#pragma once

#include <cstdio>
#include <vector>
#include <string>

#include "types.hpp"
#include "bitstream.hpp"

namespace U3D
{

class CLOD_Mesh : public Modifier
{
    //Maximum mesh description
    uint32_t attributes;
    uint32_t face_count, position_count, normal_count;
    uint32_t diffuse_count, specular_count, texcoord_count;
    struct ShadingDesc {
        uint32_t attributes;
        std::vector<uint32_t> texcoord_dims;
    };
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
    //Mesh contents
    std::vector<Vector3f> positions, normals;
    std::vector<Color4f> diffuse_colors, specular_colors;
    std::vector<TexCoord4f> texcoords;
    struct Corner
    {
        uint32_t position, normal;
        uint32_t diffuse, specular, texcoord[8];
    };
    struct Face
    {
        uint32_t shading_id;
        Corner corners[3];
    };
    struct NewFace
    {
        uint32_t shading_id;
        uint8_t ornt, third_pos_type;
        Corner corners[3];
    };
    std::vector<Face> faces;
    //Resolution update status
    uint32_t cur_res;
    Corner last_corners[3];
public:
    CLOD_Mesh(BitStreamReader& reader);
    uint32_t get_texlayer_count(uint32_t shading_id) const
    {
        return shading_descs[shading_id].texcoord_dims.size();
    }
    void create_base_mesh(BitStreamReader& reader);
    void update_resolution(BitStreamReader& reader);
};

class CLOD_Modifier : public Modifier
{
    uint32_t chain_index, attributes;
    float LOD_bias, level;
public:
    CLOD_Modifier(BitStreamReader& reader)
    {
        reader >> chain_index >> attributes >> LOD_bias >> level;
    }
};

}
