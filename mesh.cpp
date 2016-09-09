#include "mesh.hpp"

#include <algorithm>

namespace U3D
{

CLOD_Mesh::CLOD_Mesh(BitStreamReader& reader)
{
    reader.read<uint32_t>();    //Chain index is always zero.
    reader >> attributes >> face_count >> position_count >> normal_count;
    reader >> diffuse_count >> specular_count >> texcoord_count;
    uint32_t shading_count = reader.read<uint32_t>();
    shading_descs.resize(shading_count);
    for(unsigned int i = 0; i < shading_count; i++) {
        shading_descs[i].attributes = reader.read<uint32_t>();
        uint32_t texlayer_count = reader.read<uint32_t>();
        shading_descs[i].texcoord_dims.resize(texlayer_count);
        for(unsigned int j = 0; j < texlayer_count; j++) {
            shading_descs[i].texcoord_dims[j] = reader.read<uint32_t>();
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
    indexer.add_positions(position_count);
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
            for(unsigned int k = 0; k < get_texlayer_count(faces[i].shading_id); k++) {
                reader[texcoord_count] >> faces[i].corners[j].texcoord[k];
            }
        }
        indexer.add_face(i, faces[i]);
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
        /*if(split_position >= positions.size()) {
            std::fprintf(stderr, "Invalid split position.\n");
            return;
        }*/
        Color4f diffuse_average, specular_average;
        TexCoord4f texcoord_average;
        unsigned int color_match_count = 0;
        std::vector<uint32_t> split_faces = indexer.list_faces(split_position), local_positions;
        for(unsigned int j = 0; j < split_faces.size(); j++) {
            Face& face = faces[split_faces[j]];
            for(int k = 0; k < 3; k++) {
                if(face.corners[k].position == split_position) {
                    uint32_t shading_attr = shading_descs[face.shading_id].attributes;
                    if(shading_attr & 0x00000001) {
                        diffuse_average += diffuse_colors[face.corners[k].diffuse];
                    }
                    if(shading_attr & 0x00000002) {
                        specular_average += specular_colors[face.corners[k].specular];
                    }
                    if(get_texlayer_count(face.shading_id) > 0) {
                        texcoord_average += texcoords[face.corners[k].texcoord[0]];
                    }
                    color_match_count++;
                }
            }
            if(face.corners[0].position != split_position) local_positions.push_back(face.corners[0].position);
            if(face.corners[1].position != split_position) local_positions.push_back(face.corners[1].position);
            if(face.corners[2].position != split_position) local_positions.push_back(face.corners[2].position);
        }
        std::sort(local_positions.begin(), local_positions.end(), std::greater<uint32_t>());
        local_positions.erase(std::unique(local_positions.begin(), local_positions.end()), local_positions.end());
        std::fprintf(stderr, "Local position table: ");
        for(auto i : local_positions) {
            std::fprintf(stderr, "%u ", i);
        }
        std::fprintf(stderr, "\n");
        if(color_match_count > 0) {
            diffuse_average /= color_match_count;
            specular_average /= color_match_count;
            texcoord_average /= color_match_count;
        }
        uint16_t new_diffuse_count = reader[ContextEnum::cDiffuseCount].read<uint16_t>();
        std::vector<Color4f> new_diffuse_colors(new_diffuse_count, diffuse_average);
        std::fprintf(stderr, "Diffuse Count = %u.\n", new_diffuse_count);
        for(unsigned int j = 0; j < new_diffuse_count; j++) {
            uint8_t diffuse_sign = reader[ContextEnum::cDiffuseColorSign].read<uint8_t>();
            uint32_t diffuse_red = reader[ContextEnum::cColorDiffR].read<uint32_t>();
            uint32_t diffuse_green = reader[ContextEnum::cColorDiffG].read<uint32_t>();
            uint32_t diffuse_blue = reader[ContextEnum::cColorDiffB].read<uint32_t>();
            uint32_t diffuse_alpha = reader[ContextEnum::cColorDiffA].read<uint32_t>();

            new_diffuse_colors[j].r += inverse_quant(diffuse_sign & 0x1, diffuse_red, diffuse_iq);
            new_diffuse_colors[j].g += inverse_quant(diffuse_sign & 0x2, diffuse_green, diffuse_iq);
            new_diffuse_colors[j].b += inverse_quant(diffuse_sign & 0x4, diffuse_blue, diffuse_iq);
            new_diffuse_colors[j].a += inverse_quant(diffuse_sign & 0x8, diffuse_alpha, diffuse_iq);

            std::fprintf(stderr, "Diffuse [%f %f %f %f]\n", new_diffuse_colors[j].r, new_diffuse_colors[j].g, new_diffuse_colors[j].b, new_diffuse_colors[j].a);
        }
        diffuse_colors.insert(diffuse_colors.end(), new_diffuse_colors.begin(), new_diffuse_colors.end());
        uint16_t new_specular_count = reader[ContextEnum::cSpecularCount].read<uint16_t>();
        std::vector<Color4f> new_specular_colors(new_specular_count, specular_average);
        std::fprintf(stderr, "Specular Count = %u.\n", new_specular_count);
        for(unsigned int j = 0; j < new_diffuse_count; j++) {
            uint8_t specular_sign = reader[ContextEnum::cSpecularColorSign].read<uint8_t>();
            uint32_t specular_red = reader[ContextEnum::cColorDiffR].read<uint32_t>();
            uint32_t specular_green = reader[ContextEnum::cColorDiffG].read<uint32_t>();
            uint32_t specular_blue = reader[ContextEnum::cColorDiffB].read<uint32_t>();
            uint32_t specular_alpha = reader[ContextEnum::cColorDiffA].read<uint32_t>();

            new_specular_colors[j].r += inverse_quant(specular_sign & 0x1, specular_red, specular_iq);
            new_specular_colors[j].g += inverse_quant(specular_sign & 0x2, specular_green, specular_iq);
            new_specular_colors[j].b += inverse_quant(specular_sign & 0x4, specular_blue, specular_iq);
            new_specular_colors[j].a += inverse_quant(specular_sign & 0x8, specular_alpha, specular_iq);

            std::fprintf(stderr, "Specular [%f %f %f %f]\n", new_specular_colors[j].r, new_specular_colors[j].g, new_specular_colors[j].b, new_specular_colors[j].a);
        }
        specular_colors.insert(specular_colors.end(), new_specular_colors.begin(), new_specular_colors.end());
        uint16_t new_texcoord_count = reader[ContextEnum::cTexCoordCount].read<uint16_t>();
        std::vector<TexCoord4f> new_texcoords(new_texcoord_count, texcoord_average);
        std::fprintf(stderr, "TexCoord Count = %u.\n", new_texcoord_count);
        for(unsigned int j = 0; j < new_texcoord_count; j++) {
            uint8_t texcoord_sign = reader[ContextEnum::cTexCoordSign].read<uint8_t>();
            uint32_t texcoord_U = reader[ContextEnum::cTexCDiffU].read<uint32_t>();
            uint32_t texcoord_V = reader[ContextEnum::cTexCDiffV].read<uint32_t>();
            uint32_t texcoord_S = reader[ContextEnum::cTexCDiffS].read<uint32_t>();
            uint32_t texcoord_T = reader[ContextEnum::cTexCDiffT].read<uint32_t>();

            new_texcoords[j].u += inverse_quant(texcoord_sign & 0x1, texcoord_U, texcoord_iq);
            new_texcoords[j].v += inverse_quant(texcoord_sign & 0x2, texcoord_V, texcoord_iq);
            new_texcoords[j].s += inverse_quant(texcoord_sign & 0x4, texcoord_S, texcoord_iq);
            new_texcoords[j].t += inverse_quant(texcoord_sign & 0x8, texcoord_T, texcoord_iq);

            std::fprintf(stderr, "TexCoord [%f %f %f %f]\n", new_texcoords[j].u, new_texcoords[j].v, new_texcoords[j].s, new_texcoords[j].t);
        }
        texcoords.insert(texcoords.end(), new_texcoords.begin(), new_texcoords.end());
        uint32_t new_face_count = reader[ContextEnum::cFaceCnt].read<uint32_t>();
        std::vector<NewFace> new_faces(new_face_count);
        std::fprintf(stderr, "Face Count = %u.\n", new_face_count);
        for(unsigned int j = 0; j < new_face_count; j++) {
            new_faces[j].corners[0].position = split_position;
            new_faces[j].corners[1].position = positions.size();
            new_faces[j].shading_id = reader[ContextEnum::cShading].read<uint32_t>();
            new_faces[j].ornt = reader[ContextEnum::cFaceOrnt].read<uint8_t>();
            uint8_t third_pos_type = reader[ContextEnum::cThrdPosType].read<uint8_t>();
            if(third_pos_type == 1) {
                new_faces[j].corners[2].position = local_positions[reader[ContextEnum::cLocal3rdPos].read<uint32_t>()];
            } else {
                new_faces[j].corners[2].position = reader[i].read<uint32_t>();
            }
            std::fprintf(stderr, "Shading ID = %u(Attr = %u), Orientation = %u, 3rd pos = %u.\n", new_faces[j].shading_id, shading_descs[new_faces[j].shading_id].attributes, new_faces[j].ornt, new_faces[j].corners[2].position);
            uint32_t third_pos = new_faces[j].corners[2].position;
            insert_unique(local_positions, third_pos);
            std::fprintf(stderr, "Local position table: ");
            for(auto i : local_positions) {
                std::fprintf(stderr, "%u ", i);
            }
            std::fprintf(stderr, "\n");
        }
        indexer.add_position();
        std::sort(split_faces.begin(), split_faces.end(), std::greater<uint32_t>());
        std::vector<ContextEnum> predictions;
        std::vector<uint32_t> move_faces;
        for(unsigned int j = 0; j < split_faces.size(); j++) {
            Face& face = faces[split_faces[j]];
            ContextEnum context = ContextEnum::cStayMove0;
            unsigned int k;
            for(k = 0; k < new_face_count; k++) {
                uint32_t new_third = new_faces[k].corners[2].position;
                int flag = indexer.check_edge(face, split_position, new_third);
                if(flag && context != ContextEnum::cStayMove0) {
                    context = ContextEnum::cStayMove0;
                    break;
                }
                if(flag > 0) {
                    context = (new_faces[k].ornt == 1) ? ContextEnum::cStayMove1 : ContextEnum::cStayMove2;
                } else if(flag < 0) {
                    context = (new_faces[k].ornt == 1) ? ContextEnum::cStayMove2 : ContextEnum::cStayMove1;
                }
            }
            if(context == ContextEnum::cStayMove0 && k == new_face_count) {
                for(k = 0; k < j; k++) {
                    if(indexer.is_face_neighbors(face, faces[split_faces[k]])) {
                        if(context != ContextEnum::cStayMove0 && predictions[k] != ContextEnum::cStayMove0 && predictions[k] != context) {
                            context = ContextEnum::cStayMove0;
                            break;
                        } else {
                            context = predictions[k];
                        }
                    }
                }
            }
            predictions.push_back(context);
            if(context == ContextEnum::cStayMove1) {
                context = ContextEnum::cStayMove3;
            } else if(context == ContextEnum::cStayMove2) {
                context = ContextEnum::cStayMove4;
            }
            uint8_t staymove = reader[context].read<uint8_t>();
            if(staymove == 1) {
                move_faces.push_back(split_faces[j]);
            }
        }
        std::fprintf(stderr, "%u faces moved.\n", move_faces.size());
        std::vector<uint32_t> split_diffuse_colors, split_specular_colors, split_texcoords[8];
        for(unsigned int j = 0; j < split_faces.size(); j++) {
            Face& face = faces[split_faces[j]];
            for(int k = 0; k < 3; k++) {
                if(face.corners[k].position == split_position) {
                    split_diffuse_colors.push_back(face.corners[k].diffuse);
                    split_specular_colors.push_back(face.corners[k].specular);
                    for(unsigned int l = 0; l < get_texlayer_count(face.shading_id); l++) {
                        split_texcoords[l].push_back(face.corners[k].texcoord[l]);
                    }
                }
            }
        }
        greater_unique_sort(split_diffuse_colors);
        greater_unique_sort(split_specular_colors);
        for(int j = 0; j < 8; j++) {
            greater_unique_sort(split_texcoords[j]);
        }
        for(unsigned int j = 0; j < new_face_count; j++) {
            std::vector<uint32_t> third_faces = indexer.list_faces(new_faces[j].corners[2].position);
            std::vector<uint32_t> third_diffuse_colors, third_specular_colors, third_texcoords[8];
            for(unsigned int k = 0; k < third_faces.size(); k++) {
                Face& face = faces[third_faces[k]];
                for(int l = 0; l < 3; l++) {
                    if(face.corners[l].position == new_faces[j].corners[2].position) {
                        third_diffuse_colors.push_back(face.corners[l].diffuse);
                        third_specular_colors.push_back(face.corners[l].specular);
                        for(unsigned int m = 0; m < get_texlayer_count(face.shading_id); m++) {
                            third_texcoords[m].push_back(face.corners[l].texcoord[m]);
                        }
                    }
                }
            }
            greater_unique_sort(third_diffuse_colors);
            greater_unique_sort(third_specular_colors);
            for(int k = 0; k < 8; k++) {
                greater_unique_sort(third_texcoords[k]);
            }
            if(shading_descs[new_faces[j].shading_id].attributes & 0x00000001) {
                uint8_t diffuse_dup_flag = reader[ContextEnum::cColorDup].read<uint8_t>();
                for(int k = 0; k < 3; k++) {
                    if(!(diffuse_dup_flag & (1 << k))) {
                        uint8_t index_type = reader[ContextEnum::cColorIndexType].read<uint8_t>();
                        if(index_type == 2) {
                            std::fprintf(stderr, "Local diffuse table referenced.\n");
                            if(k < 2) {
                                new_faces[j].corners[k].diffuse = split_diffuse_colors[reader[ContextEnum::cColorIndexLocal].read<uint32_t>()];
                            } else {
                                new_faces[j].corners[k].diffuse = third_diffuse_colors[reader[ContextEnum::cColorIndexLocal].read<uint32_t>()];
                            }
                        } else {
                            new_faces[j].corners[k].diffuse = reader[ContextEnum::cColorIndexGlobal].read<uint32_t>();
                        }
                    } else {
                        new_faces[j].corners[k].diffuse = last_corners[k].diffuse;
                    }
                    last_corners[k].diffuse = new_faces[j].corners[k].diffuse;
                }
                insert_unique(split_diffuse_colors, new_faces[j].corners[0].diffuse);
            }
            if(shading_descs[new_faces[j].shading_id].attributes & 0x00000002) {
                uint8_t specular_dup_flag = reader[ContextEnum::cColorDup].read<uint8_t>();
                for(int k = 0; k < 3; k++) {
                    if(!(specular_dup_flag & (1 << k))) {
                        uint8_t index_type = reader[ContextEnum::cColorIndexType].read<uint8_t>();
                        if(index_type == 2) {
                            std::fprintf(stderr, "Local specular table referenced.\n");
                            if(k < 2) {
                                new_faces[j].corners[k].specular = split_specular_colors[reader[ContextEnum::cColorIndexLocal].read<uint32_t>()];
                            } else {
                                new_faces[j].corners[k].specular = third_specular_colors[reader[ContextEnum::cColorIndexLocal].read<uint32_t>()];
                            }
                        } else {
                            new_faces[j].corners[k].specular = reader[ContextEnum::cColorIndexGlobal].read<uint32_t>();
                        }
                    } else {
                        new_faces[j].corners[k].specular = last_corners[k].specular;
                    }
                    last_corners[k].specular = new_faces[j].corners[k].specular;
                }
                insert_unique(split_specular_colors, new_faces[j].corners[0].specular);
            }
            for(unsigned int k = 0; k < get_texlayer_count(new_faces[j].shading_id); k++) {
                uint8_t texcoord_dup_flag = reader[ContextEnum::cTexCDup].read<uint8_t>();
                for(int l = 0; l < 3; l++) {
                    if(!(texcoord_dup_flag & (1 << l))) {
                        uint8_t index_type = reader[ContextEnum::cTexCIndexType].read<uint8_t>();
                        if(index_type == 2) {
                            std::fprintf(stderr, "Local texcoord table referenced.\n");
                            if(l < 2) {
                                new_faces[j].corners[l].texcoord[k] = split_texcoords[k][reader[ContextEnum::cTextureIndexLocal].read<uint32_t>()];
                            } else {
                                new_faces[j].corners[l].texcoord[k] = third_texcoords[k][reader[ContextEnum::cTextureIndexLocal].read<uint32_t>()];
                            }
                        } else {
                            new_faces[j].corners[l].texcoord[k] = reader[ContextEnum::cTextureIndexGlobal].read<uint32_t>();
                        }
                    } else {
                        new_faces[j].corners[l].texcoord[k] = last_corners[l].texcoord[k];
                    }
                    last_corners[l].texcoord[k] = new_faces[j].corners[l].texcoord[k];
                }
                insert_unique(split_texcoords[k], new_faces[j].corners[0].texcoord[k]);
            }
            Face face;
            face.shading_id = new_faces[j].shading_id;
            if(new_faces[j].ornt == 1) {
                face.corners[0] = new_faces[j].corners[0];
                face.corners[1] = new_faces[j].corners[1];
            } else {
                face.corners[0] = new_faces[j].corners[1];
                face.corners[1] = new_faces[j].corners[0];
            }
            face.corners[2] = new_faces[j].corners[2];
            faces.push_back(face);
            indexer.add_face(faces.size() - 1, face);
            std::fprintf(stderr, "New face: %u %u %u\n", face.corners[0].position, face.corners[1].position, face.corners[2].position);
        }
        Vector3f new_position;
        if(split_position < positions.size()) {
            new_position = positions[split_position];
        }
        std::fprintf(stderr, "Predicted Position [%f %f %f]\n", new_position.x, new_position.y, new_position.z);
        uint8_t pos_sign = reader[ContextEnum::cPosDiffSign].read<uint8_t>();
        uint32_t pos_X = reader[ContextEnum::cPosDiffX].read<uint32_t>();
        uint32_t pos_Y = reader[ContextEnum::cPosDiffY].read<uint32_t>();
        uint32_t pos_Z = reader[ContextEnum::cPosDiffZ].read<uint32_t>();
        new_position.x += inverse_quant(pos_sign & 0x1, pos_X, position_iq);
        new_position.y += inverse_quant(pos_sign & 0x2, pos_Y, position_iq);
        new_position.z += inverse_quant(pos_sign & 0x4, pos_Z, position_iq);
        std::fprintf(stderr, "New Position [%u %u %u]\n", pos_X, pos_Y, pos_Z);
        std::fprintf(stderr, "New Position [%f %f %f]\n", new_position.x, new_position.y, new_position.z);
        positions.push_back(new_position);
        if(!(attributes & 0x00000001)) {
            std::vector<uint32_t> neighbors = indexer.list_inclusive_neighbors(faces, static_cast<uint32_t>(positions.size() - 1));
            print_vector(neighbors, "Neighbors");
            for(unsigned int j = 0; j < neighbors.size(); j++) {
                uint32_t normal_count = reader[ContextEnum::cNormalCnt].read<uint32_t>();
                for(unsigned int k = 0; k < normal_count; k++) {
                    uint8_t normal_sign = reader[ContextEnum::cDiffNormalSign].read<uint8_t>();
                    uint32_t normal_X = reader[ContextEnum::cDiffNormalX].read<uint32_t>();
                    uint32_t normal_Y = reader[ContextEnum::cDiffNormalY].read<uint32_t>();
                    uint32_t normal_Z = reader[ContextEnum::cDiffNormalZ].read<uint32_t>();
                }
                std::vector<uint32_t> client_faces = indexer.list_faces(neighbors[j]);
                for(unsigned int k = 0; k < client_faces.size(); k++) {
                    uint32_t normal_index = reader[ContextEnum::cNormalIdx].read<uint32_t>();
                }
            }
        }
    }
    cur_res = end;
}

}
