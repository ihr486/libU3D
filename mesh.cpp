#include "mesh.hpp"

namespace U3D
{

CLOD_Mesh::CLOD_Mesh(BitStreamReader& reader)
{
    reader >> chain_index;
    reader >> maxmesh.attributes >> maxmesh.face_count >> maxmesh.position_count >> maxmesh.normal_count;
    reader >> maxmesh.diffuse_count >> maxmesh.specular_count >> maxmesh.texcoord_count;
    uint32_t shading_count = reader.read<uint32_t>();
    maxmesh.shading_descs.resize(shading_count);
    for(unsigned int i = 0; i < shading_count; i++) {
        maxmesh.shading_descs[i].attributes = reader.read<uint32_t>();
        uint32_t texlayer_count = reader.read<uint32_t>();
        maxmesh.shading_descs[i].texcoord_dims.resize(texlayer_count);
        for(unsigned int j = 0; j < texlayer_count; j++) {
            maxmesh.shading_descs[i].texcoord_dims[j] = reader.read<uint32_t>();
        }
        reader.read<uint32_t>();
    }
    reader >> min_res >> max_res;
    reader >> position_quality >> normal_quality >> texcoord_quality;
    reader >> position_iq >> normal_iq >> texcoord_iq >> diffuse_iq >> specular_iq;
    reader >> normal_crease >> normal_update >> normal_tolerance;
    uint32_t bone_count = reader.read<uint32_t>();
    skeleton.resize(bone_count);
    for(unsigned int i = 0; i < bone_count; i++) {
        reader >> skeleton[i].name >> skeleton[i].parent_name >> skeleton[i].attributes;
        reader >> skeleton[i].length >> skeleton[i].displacement >> skeleton[i].orientation;
        if(skeleton[i].attributes & 0x00000001) {
            reader >> skeleton[i].link_count >> skeleton[i].link_length;
        }
        if(skeleton[i].attributes & 0x00000002) {
            reader >> skeleton[i].start_joint_center >> skeleton[i].start_joint_scale;
            reader >> skeleton[i].end_joint_center >> skeleton[i].end_joint_scale;
        }
        for(int j = 0; j < 6; j++) {
            reader.read<float>();   //Skip past the rotation constraints
        }
    }
    cur_res = 0;
}

void CLOD_Mesh::create_base_mesh(BitStreamReader& reader)
{
    uint32_t face_count, position_count, normal_count;
    uint32_t diffuse_count, specular_count, texcoord_count;
    reader.read<uint32_t>();    //Chain index is always zero.
    reader >> face_count >> position_count >> normal_count >> diffuse_count >> specular_count >> texcoord_count;
    if(cur_res > 0 || min_res != position_count) {
        std::fprintf(stderr, "Base mesh is already set up.\n");
        return;
    }
    positions.resize(position_count);
    for(unsigned int i = 0; i < position_count; i++) reader >> positions[i];
    normals.resize(normal_count);
    for(unsigned int i = 0; i < normal_count; i++) reader >> normals[i];
    diffuse_colors.resize(diffuse_count);
    for(unsigned int i = 0; i < diffuse_count; i++) reader >> diffuse_colors[i];
    specular_colors.resize(specular_count);
    for(unsigned int i = 0; i < specular_count; i++) reader >> specular_colors[i];
    texcoords.resize(texcoord_count);
    for(unsigned int i = 0; i < texcoord_count; i++) reader >> texcoords[i];
    faces.resize(face_count);
    for(unsigned int i = 0; i < face_count; i++) {
        reader[ContextEnum::cShading] >> faces[i].shading_id;
        for(int j = 0; j < 3; j++) {
            reader[position_count] >> faces[i].corners[j].position;
            reader[normal_count] >> faces[i].corners[j].normal;
            reader[diffuse_count] >> faces[i].corners[j].diffuse;
            reader[specular_count] >> faces[i].corners[j].specular;
            for(int k = 0; k < get_texlayer_count(faces[i].shading_id); k++) {
                reader[texcoord_count] >> faces[i].corners[j].texcoord[k];
            }
        }
    }
    cur_res = min_res;
}

void CLOD_Mesh::update_resolution(BitStreamReader& reader)
{
    uint32_t start, end;
    reader.read<uint32_t>();    //Chain index is always zero.
    reader >> start >> end;
    if(cur_res != start) {
        std::fprintf(stderr, "Resolution Updates seem badly ordered.\n");
        return;
    }
    for(unsigned int i = start; i < end; i++) {
        std::fprintf(stderr, "Resolution update %u\n", i);
        uint32_t split_position;
        if(i == 0) {
            reader[ContextEnum::cZero] >> split_position;
        } else {
            reader[i] >> split_position;
        }
        std::fprintf(stderr, "Split Position = %u.\n", split_position);
        uint16_t new_diffuse_count = reader[ContextEnum::cDiffuseCount].read<uint16_t>();
        std::fprintf(stderr, "Diffuse Count = %u.\n", new_diffuse_count);
        for(unsigned int j = 0; j < new_diffuse_count; j++) {
            uint8_t diffuse_sign = reader[ContextEnum::cDiffuseColorSign].read<uint8_t>();
            uint32_t diffuse_red = reader[ContextEnum::cColorDiffR].read<uint32_t>();
            uint32_t diffuse_green = reader[ContextEnum::cColorDiffG].read<uint32_t>();
            uint32_t diffuse_blue = reader[ContextEnum::cColorDiffB].read<uint32_t>();
            uint32_t diffuse_alpha = reader[ContextEnum::cColorDiffA].read<uint32_t>();

            std::fprintf(stderr, "%u %u %u %u %u\n", diffuse_sign, diffuse_red, diffuse_green, diffuse_blue, diffuse_alpha);
        }
        uint16_t new_specular_count = reader[ContextEnum::cSpecularCount].read<uint16_t>();
        std::fprintf(stderr, "Specular Count = %u.\n", new_specular_count);
        for(unsigned int j = 0; j < new_diffuse_count; j++) {
            uint8_t specular_sign = reader[ContextEnum::cSpecularColorSign].read<uint8_t>();
            uint32_t specular_red = reader[ContextEnum::cColorDiffR].read<uint32_t>();
            uint32_t specular_green = reader[ContextEnum::cColorDiffG].read<uint32_t>();
            uint32_t specular_blue = reader[ContextEnum::cColorDiffB].read<uint32_t>();
            uint32_t specular_alpha = reader[ContextEnum::cColorDiffA].read<uint32_t>();

            std::fprintf(stderr, "%u %u %u %u %u\n", specular_sign, specular_red, specular_green, specular_blue, specular_alpha);
        }
        uint16_t new_texcoord_count = reader[ContextEnum::cTexCoordCount].read<uint16_t>();
        std::fprintf(stderr, "TexCoord Count = %u.\n", new_texcoord_count);
        for(unsigned int j = 0; j < new_texcoord_count; j++) {
            uint8_t texcoord_sign = reader[ContextEnum::cTexCoordSign].read<uint8_t>();
            uint32_t texcoord_U = reader[ContextEnum::cTexCDiffU].read<uint32_t>();
            uint32_t texcoord_V = reader[ContextEnum::cTexCDiffV].read<uint32_t>();
            uint32_t texcoord_S = reader[ContextEnum::cTexCDiffS].read<uint32_t>();
            uint32_t texcoord_T = reader[ContextEnum::cTexCDiffT].read<uint32_t>();

            std::fprintf(stderr, "%u %u %u %u %u\n", texcoord_sign, texcoord_U, texcoord_V, texcoord_S, texcoord_T);
        }
        uint32_t new_face_count = reader[ContextEnum::cFaceCnt].read<uint32_t>();
        std::fprintf(stderr, "Face Count = %u.\n", new_face_count);
        for(unsigned int j = 0; j < new_face_count; j++) {
            uint32_t shading_id = reader[ContextEnum::cShading].read<uint32_t>();
            uint8_t face_ornt = reader[ContextEnum::cFaceOrnt].read<uint8_t>();
            uint8_t third_pos_type = reader[ContextEnum::cThrdPosType].read<uint8_t>();
            uint32_t third_pos;
            if(third_pos_type == 1) {
                third_pos = reader[ContextEnum::cLocal3rdPos].read<uint32_t>();
            } else {
                third_pos = reader[i].read<uint32_t>();
            }
            std::fprintf(stderr, "Shading ID = %u, Orientation = %u, 3rd pos = %u.\n", shading_id, face_ornt, third_pos);
        }
        for(unsigned int j = 0; j < new_face_count; j++) {
            /*if(mesh.is_diffuse_color_present(shading_id)) {
            }
            if(mesh.is_specular_color_present(shading_id)) {
            }
            if(mesh.get_texlayer_count(shading_id) > 0) {
            }*/
        }
        uint8_t pos_sign = reader[ContextEnum::cPosDiffSign].read<uint8_t>();
        uint32_t pos_X = reader[ContextEnum::cPosDiffX].read<uint32_t>();
        uint32_t pos_Y = reader[ContextEnum::cPosDiffY].read<uint32_t>();
        uint32_t pos_Z = reader[ContextEnum::cPosDiffZ].read<uint32_t>();
        std::fprintf(stderr, "New Position = %u %u %u %u\n", pos_sign, pos_X, pos_Y, pos_Z);
        /*if(!mesh.is_normals_excluded()) {
          uint32_t new_normal_count = reader[ContextEnum::cNormalCnt].read<uint32_t>();
          std::fprintf(stderr, "Normal Count = %u.\n", new_normal_count);
          for(unsigned int j = 0; j < new_normal_count; j++) {
          uint8_t normal_sign = reader[ContextEnum::cDiffNormalSign].read<uint8_t>();
          uint32_t normal_X = reader[ContextEnum::cDiffNormalX].read<uint32_t>();
          uint32_t normal_Y = reader[ContextEnum::cDiffNormalY].read<uint32_t>();
          uint32_t normal_Z = reader[ContextEnum::cDiffNormalZ].read<uint32_t>();
          }
          }
          return;*/
    }
    cur_res = end;
}

}
