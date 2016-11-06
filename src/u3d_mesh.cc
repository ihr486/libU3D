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

#include "u3d_internal.hh"

namespace U3D
{

CLOD_Object::CLOD_Object(bool clod_desc_flag, BitStreamReader& reader)
{
    reader.read<uint32_t>();    //Chain index is always zero.
    reader >> attributes >> face_count >> position_count >> normal_count;
    reader >> diffuse_count >> specular_count >> texcoord_count;
    uint32_t shading_count = reader.read<uint32_t>();
    shading_descs.resize(shading_count);
    for(unsigned int i = 0; i < shading_count; i++) {
        shading_descs[i].attributes = reader.read<uint32_t>();
        shading_descs[i].texlayer_count = reader.read<uint32_t>();
        for(unsigned int j = 0; j < shading_descs[i].texlayer_count; j++) {
            shading_descs[i].texcoord_dims[j] = reader.read<uint32_t>();
        }
        reader.read<uint32_t>();
    }
    if(clod_desc_flag) {
        reader >> min_res >> max_res;
    }
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
}

CLOD_Mesh::CLOD_Mesh(BitStreamReader& reader) : CLOD_Object(true, reader)
{
    cur_res = 0;
    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 8; j++) {
            last_corners[i].texcoord[j] = 0;
        }
    }
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
        reader[cShading] >> faces[i].shading_id;
        for(int j = 0; j < 3; j++) {
            reader[position_count] >> faces[i].corners[j].position;
            if(!(attributes & 0x00000001)) {
                reader[normal_count] >> faces[i].corners[j].normal;
            }
            if(shading_descs[faces[i].shading_id].attributes & 0x00000001) {
                reader[diffuse_count] >> faces[i].corners[j].diffuse;
            }
            if(shading_descs[faces[i].shading_id].attributes & 0x00000002) {
                reader[specular_count] >> faces[i].corners[j].specular;
            }
            for(unsigned int k = 0; k < shading_descs[faces[i].shading_id].texlayer_count; k++) {
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
        uint32_t split_position;
        if(i == 0) {
            reader[cZero] >> split_position;
        } else {
            reader[i] >> split_position;
        }
        Color4f diffuse_average, specular_average;
        TexCoord4f texcoord_average;
        unsigned int color_match_count = 0;
        std::vector<uint32_t> split_faces = indexer.list_faces(split_position), local_positions;
        for(unsigned int j = 0; j < split_faces.size(); j++) {
            Face& face = faces[split_faces[j]];
            Corner& corner = face.get_corner(split_position);
            uint32_t shading_attr = shading_descs[face.shading_id].attributes;
            if(shading_attr & 0x00000001) {
                diffuse_average += diffuse_colors[corner.diffuse];
            }
            if(shading_attr & 0x00000002) {
                specular_average += specular_colors[corner.specular];
            }
            if(shading_descs[face.shading_id].texlayer_count > 0) {
                texcoord_average += texcoords[corner.texcoord[0]];
            }
            color_match_count++;
            if(face.corners[0].position != split_position) local_positions.push_back(face.corners[0].position);
            if(face.corners[1].position != split_position) local_positions.push_back(face.corners[1].position);
            if(face.corners[2].position != split_position) local_positions.push_back(face.corners[2].position);
        }
        greater_unique_sort(local_positions);
        if(color_match_count > 0) {
            diffuse_average /= color_match_count;
            specular_average /= color_match_count;
            texcoord_average /= color_match_count;
        }
        uint16_t new_diffuse_count = reader[cDiffuseCount].read<uint16_t>();
        std::vector<Color4f> new_diffuse_colors(new_diffuse_count, diffuse_average);
        for(unsigned int j = 0; j < new_diffuse_count; j++) {
            uint8_t diffuse_sign = reader[cDiffuseColorSign].read<uint8_t>();
            uint32_t diffuse_red = reader[cColorDiffR].read<uint32_t>();
            uint32_t diffuse_green = reader[cColorDiffG].read<uint32_t>();
            uint32_t diffuse_blue = reader[cColorDiffB].read<uint32_t>();
            uint32_t diffuse_alpha = reader[cColorDiffA].read<uint32_t>();

            new_diffuse_colors[j] += Color4f::dequantize(diffuse_sign, diffuse_red, diffuse_green, diffuse_blue, diffuse_alpha, diffuse_iq);
        }
        uint16_t new_specular_count = reader[cSpecularCount].read<uint16_t>();
        std::vector<Color4f> new_specular_colors(new_specular_count, specular_average);
        for(unsigned int j = 0; j < new_diffuse_count; j++) {
            uint8_t specular_sign = reader[cSpecularColorSign].read<uint8_t>();
            uint32_t specular_red = reader[cColorDiffR].read<uint32_t>();
            uint32_t specular_green = reader[cColorDiffG].read<uint32_t>();
            uint32_t specular_blue = reader[cColorDiffB].read<uint32_t>();
            uint32_t specular_alpha = reader[cColorDiffA].read<uint32_t>();

            new_specular_colors[j] += Color4f::dequantize(specular_sign, specular_red, specular_green, specular_blue, specular_alpha, specular_iq);
        }
        uint16_t new_texcoord_count = reader[cTexCoordCount].read<uint16_t>();
        std::vector<TexCoord4f> new_texcoords(new_texcoord_count, texcoord_average);
        for(unsigned int j = 0; j < new_texcoord_count; j++) {
            uint8_t texcoord_sign = reader[cTexCoordSign].read<uint8_t>();
            uint32_t texcoord_U = reader[cTexCDiffU].read<uint32_t>();
            uint32_t texcoord_V = reader[cTexCDiffV].read<uint32_t>();
            uint32_t texcoord_S = reader[cTexCDiffS].read<uint32_t>();
            uint32_t texcoord_T = reader[cTexCDiffT].read<uint32_t>();

            new_texcoords[j] += TexCoord4f::dequantize(texcoord_sign, texcoord_U, texcoord_V, texcoord_S, texcoord_T, texcoord_iq);
        }
        uint32_t new_face_count = reader[cFaceCnt].read<uint32_t>();
        std::vector<NewFace> new_faces(new_face_count);
        for(unsigned int j = 0; j < new_face_count; j++) {
            new_faces[j].corners[0].position = split_position;
            new_faces[j].corners[1].position = positions.size();
            new_faces[j].shading_id = reader[cShading].read<uint32_t>();
            new_faces[j].ornt = reader[cFaceOrnt].read<uint8_t>();
            uint8_t third_pos_type = reader[cThrdPosType].read<uint8_t>();
            if(third_pos_type == 1) {
                new_faces[j].corners[2].position = local_positions[reader[cLocal3rdPos].read<uint32_t>()];
            } else {
                new_faces[j].corners[2].position = reader[i].read<uint32_t>();
            }
            uint32_t third_pos = new_faces[j].corners[2].position;
            insert_unique(local_positions, third_pos);
        }
        indexer.add_position();
        std::sort(split_faces.begin(), split_faces.end(), std::greater<uint32_t>());
        std::vector<uint32_t> split_diffuse_colors, split_specular_colors, split_texcoords[8];
        for(unsigned int j = 0; j < split_faces.size(); j++) {
            Face& face = faces[split_faces[j]];
            Corner& corner = face.get_corner(split_position);
            split_diffuse_colors.push_back(corner.diffuse);
            split_specular_colors.push_back(corner.specular);
            for(unsigned int l = 0; l < shading_descs[face.shading_id].texlayer_count; l++) {
                split_texcoords[l].push_back(corner.texcoord[l]);
            }
        }
        greater_unique_sort(split_diffuse_colors);
        greater_unique_sort(split_specular_colors);
        for(int j = 0; j < 8; j++) {
            greater_unique_sort(split_texcoords[j]);
        }
        std::vector<uint32_t> move_faces, moved_positions, stayed_positions;
        for(unsigned int j = 0; j < split_faces.size(); j++) {
            Face& face = faces[split_faces[j]];
            ContextEnum context = cStayMove0;
            for(unsigned int k = 0; k < new_face_count; k++) {
                uint32_t new_third = new_faces[k].corners[2].position;
                int flag = indexer.check_edge(face, split_position, new_third);
                if(flag > 0) {
                    context = (new_faces[k].ornt == 1) ? cStayMove1 : cStayMove2;
                    break;
                } else if(flag < 0) {
                    context = (new_faces[k].ornt == 1) ? cStayMove2 : cStayMove1;
                    break;
                }
            }
            if(context == cStayMove0) {
                for(int k = 0; k < 3; k++) {
                    if(std::find(moved_positions.begin(), moved_positions.end(), face.corners[k].position) != moved_positions.end()) {
                        context = cStayMove3;
                        break;
                    }
                }
            }
            if(context == cStayMove0) {
                for(int k = 0; k < 3; k++) {
                    if(std::find(stayed_positions.begin(), stayed_positions.end(), face.corners[k].position) != stayed_positions.end()) {
                        context = cStayMove4;
                        break;
                    }
                }
            }
            uint8_t staymove = reader[context].read<uint8_t>();
            if(staymove == 1) {
                move_faces.push_back(split_faces[j]);
                for(int k = 0; k < 3; k++) {
                    if(face.corners[k].position != split_position) moved_positions.push_back(face.corners[k].position);
                }
            } else {
                for(int k = 0; k < 3; k++) {
                    if(face.corners[k].position != split_position) stayed_positions.push_back(face.corners[k].position);
                }
            }
        }
        for(unsigned int j = 0; j < move_faces.size(); j++) {
            Face& face = faces[move_faces[j]];
            Corner& corner = face.get_corner(split_position);
            if(shading_descs[face.shading_id].attributes & 0x00000001) {
                uint8_t keep_change = reader[cDiffuseKeepChange].read<uint8_t>();
                if(keep_change == 0x1) {
                    uint8_t change_type = reader[cDiffuseChangeType].read<uint8_t>();
                    uint32_t new_index;
                    if(change_type == 0x1) {
                        new_index = diffuse_colors.size() + reader[cDiffuseChangeIndexNew].read<uint32_t>();
                    } else if(change_type == 0x2) {
                        uint32_t local_index = reader[cDiffuseChangeIndexLocal].read<uint32_t>();
                        std::vector<uint32_t> split_diffuse_colors = indexer.list_diffuse_colors(faces, split_position);
                        new_index = split_diffuse_colors[local_index];
                    } else {
                        new_index = reader[cDiffuseChangeIndexGlobal].read<uint32_t>();
                    }
                    corner.diffuse = new_index;
                }
            }
            if(shading_descs[face.shading_id].attributes & 0x00000002) {
                uint8_t keep_change = reader[cSpecularKeepChange].read<uint8_t>();
                if(keep_change == 0x1) {
                    uint8_t change_type = reader[cSpecularChangeType].read<uint8_t>();
                    uint32_t new_index;
                    if(change_type == 0x1) {
                        new_index = specular_colors.size() + reader[cSpecularChangeIndexNew].read<uint32_t>();
                    } else if(change_type == 0x2) {
                        uint32_t local_index = reader[cSpecularChangeIndexLocal].read<uint32_t>();
                        std::vector<uint32_t> split_specular_colors = indexer.list_specular_colors(faces, split_position);
                        new_index = split_specular_colors[local_index];
                    } else {
                        new_index = reader[cSpecularChangeIndexGlobal].read<uint32_t>();
                    }
                    corner.specular = new_index;
                }
            }
            for(unsigned int k = 0; k < shading_descs[face.shading_id].texlayer_count; k++) {
                uint8_t keep_change = reader[cTCKeepChange].read<uint8_t>();
                if(keep_change == 0x1) {
                    uint8_t change_type = reader[cTCChangeType].read<uint8_t>();
                    uint32_t new_index;
                    if(change_type == 0x1) {
                        new_index = texcoords.size() + reader[cTCChangeIndexNew].read<uint32_t>();
                    } else if(change_type == 0x2) {
                        uint32_t local_index = reader[cTCChangeIndexLocal].read<uint32_t>();
                        std::vector<uint32_t> split_texcoords = indexer.list_texcoords(faces, shading_descs, split_position, k);
                        new_index = split_texcoords[local_index];
                    } else {
                        new_index = reader[cTCChangeIndexGlobal].read<uint32_t>();
                    }
                    corner.texcoord[k] = new_index;
                }
            }
            face.get_corner(split_position).position = positions.size();
            indexer.move_position(move_faces[j], split_position, positions.size());
        }
        diffuse_colors.insert(diffuse_colors.end(), new_diffuse_colors.begin(), new_diffuse_colors.end());
        specular_colors.insert(specular_colors.end(), new_specular_colors.begin(), new_specular_colors.end());
        texcoords.insert(texcoords.end(), new_texcoords.begin(), new_texcoords.end());
        for(unsigned int j = 0; j < new_face_count; j++) {
            std::vector<uint32_t> third_faces = indexer.list_faces(new_faces[j].corners[2].position);
            std::vector<uint32_t> third_diffuse_colors, third_specular_colors, third_texcoords[8];
            for(unsigned int k = 0; k < third_faces.size(); k++) {
                Face& face = faces[third_faces[k]];
                Corner& corner = face.get_corner(new_faces[j].corners[2].position);
                third_diffuse_colors.push_back(corner.diffuse);
                third_specular_colors.push_back(corner.specular);
                for(unsigned int m = 0; m < shading_descs[face.shading_id].texlayer_count; m++) {
                    third_texcoords[m].push_back(corner.texcoord[m]);
                }
            }
            greater_unique_sort(third_diffuse_colors);
            greater_unique_sort(third_specular_colors);
            for(int k = 0; k < 8; k++) {
                greater_unique_sort(third_texcoords[k]);
            }
            if(shading_descs[new_faces[j].shading_id].attributes & 0x00000001) {
                uint8_t diffuse_dup_flag = reader[cColorDup].read<uint8_t>();
                for(int k = 0; k < 3; k++) {
                    if(!(diffuse_dup_flag & (1 << k))) {
                        uint8_t index_type = reader[cColorIndexType].read<uint8_t>();
                        if(index_type == 2) {
                            if(k < 2) {
                                new_faces[j].corners[k].diffuse = split_diffuse_colors[reader[cColorIndexLocal].read<uint32_t>()];
                            } else {
                                new_faces[j].corners[k].diffuse = third_diffuse_colors[reader[cColorIndexLocal].read<uint32_t>()];
                            }
                        } else {
                            new_faces[j].corners[k].diffuse = reader[cColorIndexGlobal].read<uint32_t>();
                        }
                    } else {
                        new_faces[j].corners[k].diffuse = last_corners[k].diffuse;
                    }
                    last_corners[k].diffuse = new_faces[j].corners[k].diffuse;
                    if(k == 0) insert_unique(split_diffuse_colors, new_faces[j].corners[0].diffuse);
                }
            }
            if(shading_descs[new_faces[j].shading_id].attributes & 0x00000002) {
                uint8_t specular_dup_flag = reader[cColorDup].read<uint8_t>();
                for(int k = 0; k < 3; k++) {
                    if(!(specular_dup_flag & (1 << k))) {
                        uint8_t index_type = reader[cColorIndexType].read<uint8_t>();
                        if(index_type == 2) {
                            if(k < 2) {
                                new_faces[j].corners[k].specular = split_specular_colors[reader[cColorIndexLocal].read<uint32_t>()];
                            } else {
                                new_faces[j].corners[k].specular = third_specular_colors[reader[cColorIndexLocal].read<uint32_t>()];
                            }
                        } else {
                            new_faces[j].corners[k].specular = reader[cColorIndexGlobal].read<uint32_t>();
                        }
                    } else {
                        new_faces[j].corners[k].specular = last_corners[k].specular;
                    }
                    last_corners[k].specular = new_faces[j].corners[k].specular;
                    if(k == 0) insert_unique(split_specular_colors, new_faces[j].corners[0].specular);
                }
            }
            for(unsigned int k = 0; k < shading_descs[new_faces[j].shading_id].texlayer_count; k++) {
                uint8_t texcoord_dup_flag = reader[cTexCDup].read<uint8_t>();
                for(int l = 0; l < 3; l++) {
                    if(!(texcoord_dup_flag & (1 << l))) {
                        uint8_t index_type = reader[cTexCIndexType].read<uint8_t>();
                        if(index_type == 2) {
                            if(l < 2) {
                                new_faces[j].corners[l].texcoord[k] = split_texcoords[k][reader[cTextureIndexLocal].read<uint32_t>()];
                            } else {
                                new_faces[j].corners[l].texcoord[k] = third_texcoords[k][reader[cTextureIndexLocal].read<uint32_t>()];
                            }
                        } else {
                            new_faces[j].corners[l].texcoord[k] = reader[cTextureIndexGlobal].read<uint32_t>();
                        }
                    } else {
                        new_faces[j].corners[l].texcoord[k] = last_corners[l].texcoord[0];
                    }
                    last_corners[l].texcoord[0] = new_faces[j].corners[l].texcoord[k];
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
        }
        Vector3f new_position;
        if(split_position < positions.size()) {
            new_position = positions[split_position];
        }
        uint8_t pos_sign = reader[cPosDiffSign].read<uint8_t>();
        uint32_t pos_X = reader[cPosDiffX].read<uint32_t>();
        uint32_t pos_Y = reader[cPosDiffY].read<uint32_t>();
        uint32_t pos_Z = reader[cPosDiffZ].read<uint32_t>();
        new_position += Vector3f::dequantize(pos_sign, pos_X, pos_Y, pos_Z, position_iq);
        positions.push_back(new_position);
        if(!(attributes & 0x00000001)) {
            std::vector<uint32_t> neighbors = indexer.list_inclusive_neighbors(faces, static_cast<uint32_t>(positions.size() - 1));
            for(unsigned int j = 0; j < neighbors.size(); j++) {
                uint32_t normal_count = reader[cNormalCnt].read<uint32_t>();
                std::vector<Vector3f> face_norms, new_norms;
                std::vector<uint32_t> client_faces = indexer.list_faces(neighbors[j]);
                for(unsigned int k = 0; k < client_faces.size(); k++) {
                    Face& face = faces[client_faces[k]];
                    Vector3f ba = positions[face.corners[1].position] - positions[face.corners[0].position];
                    Vector3f ca = positions[face.corners[2].position] - positions[face.corners[0].position];
                    Vector3f n0 = (ba ^ ca).normalize();
                    face_norms.push_back(n0);
                }
                new_norms.push_back(face_norms[0]);
                while(new_norms.size() < normal_count) {
                    float farthest_dist = 1.0f;
                    std::vector<Vector3f>::iterator farthest_index = face_norms.begin();
                    for(std::vector<Vector3f>::iterator k = face_norms.begin(); k != face_norms.end(); k++) {
                        float nearest_dist = -1.0f;
                        for(unsigned int l = 0; l < new_norms.size(); l++) {
                            if((*k) * new_norms[l] > nearest_dist) {
                                nearest_dist = (*k) * new_norms[l];
                            }
                        }
                        if(nearest_dist < farthest_dist) {
                            farthest_dist = nearest_dist;
                            farthest_index = k;
                        }
                    }
                    new_norms.push_back(*farthest_index);
                    face_norms.erase(farthest_index);
                }
                std::vector<unsigned int> merge_weight(new_norms.size(), 0);
                while(face_norms.size() > 0) {
                    float nearest_dist = -1.0f;
                    unsigned int nearest_index = 0;
                    for(unsigned int k = 0; k < new_norms.size(); k++) {
                        if(new_norms[k] * face_norms.back() > nearest_dist) {
                            nearest_dist = new_norms[k] * face_norms.back();
                            nearest_index = k;
                        }
                    }
                    new_norms[nearest_index] = slerp(new_norms[nearest_index], face_norms.back(), 1.0f / (merge_weight[nearest_index] + 2.0f));
                    merge_weight[nearest_index]++;
                    face_norms.pop_back();
                }
                for(unsigned int k = 0; k < normal_count; k++) {
                    uint8_t normal_sign = reader[cDiffNormalSign].read<uint8_t>();
                    uint32_t normal_X = reader[cDiffNormalX].read<uint32_t>();
                    uint32_t normal_Y = reader[cDiffNormalY].read<uint32_t>();
                    uint32_t normal_Z = reader[cDiffNormalZ].read<uint32_t>();

                    Quaternion4f normal_diff(Vector3f::dequantize(normal_sign >> 1, normal_X, normal_Y, normal_Z, normal_iq));
                    normal_diff.w = sqrtf(1.0 - std::min(1.0f, normal_diff.x * normal_diff.x + normal_diff.y * normal_diff.y + normal_diff.z * normal_diff.z));

                    new_norms[k] = normal_diff * Quaternion4f(new_norms[k]);
                }
                for(unsigned int k = 0; k < client_faces.size(); k++) {
                    uint32_t normal_index = normals.size() + reader[cNormalIdx].read<uint32_t>();

                    faces[client_faces[k]].get_corner(neighbors[j]).normal = normal_index;
                }
                normals.insert(normals.end(), new_norms.begin(), new_norms.end());
            }
        }
    }
    cur_res = end;
}

void CLOD_Mesh::dump_author_mesh()
{
    for(unsigned int i = 0; i < faces.size(); i++) {
        std::printf("Face #%u [Shading ID = %u]\n", i, faces[i].shading_id);
        Face& face = faces[i];
        uint32_t attr = shading_descs[face.shading_id].attributes;
        for(int j = 0; j < 3; j++) {
            std::printf("\tCorner #%d\n", j);
            Corner& corner = face.corners[j];
            std::printf("\t\tPosition %u [%f %f %f]\n", corner.position, positions[corner.position].x, positions[corner.position].y, positions[corner.position].z);
            std::printf("\t\tNormal %u [%f %f %f]\n", corner.normal, normals[corner.normal].x, normals[corner.normal].y, normals[corner.normal].z);
            if(attr & 0x00000001)
                std::printf("\t\tDiffuse %u [%f %f %f %f]\n", corner.diffuse, diffuse_colors[corner.diffuse].r, diffuse_colors[corner.diffuse].g, diffuse_colors[corner.diffuse].b, diffuse_colors[corner.diffuse].a);
            if(attr & 0x00000002)
                std::printf("\t\tSpecular %u [%f %f %f %f]\n", corner.specular, specular_colors[corner.specular].r, specular_colors[corner.specular].g, specular_colors[corner.specular].b, specular_colors[corner.specular].a);
            unsigned int layers = shading_descs[face.shading_id].texlayer_count;
            for(unsigned int k = 0; k < layers; k++) {
                std::printf("\t\tTexCoord #%u %u [%f %f %f %f]\n", k, corner.texcoord[k], texcoords[corner.texcoord[k]].u, texcoords[corner.texcoord[k]].v, texcoords[corner.texcoord[k]].s, texcoords[corner.texcoord[k]].t);
            }
        }
    }
}

RenderGroup *CLOD_Mesh::create_render_group()
{
    RenderGroup *group = new RenderGroup(GL_TRIANGLES, shading_descs.size());
    std::vector<int> face_count(shading_descs.size());
    for(unsigned int i = 0; i < faces.size(); i++) {
        face_count[faces[i].shading_id]++;
    }
    for(unsigned int i = 0; i < shading_descs.size(); i++) {
        uint32_t flags = RenderGroup::BUFFER_POSITION_MASK;
        if(!(attributes & EXCLUDE_NORMALS)) {
            flags |= RenderGroup::BUFFER_NORMAL_MASK;
        }
        if(shading_descs[i].attributes & VERTEX_DIFFUSE_COLOR) {
            flags |= RenderGroup::BUFFER_DIFFUSE_MASK;
        }
        if(shading_descs[i].attributes & VERTEX_SPECULAR_COLOR) {
            flags |= RenderGroup::BUFFER_SPECULAR_MASK;
        }
        for(unsigned int j = 0; j < shading_descs[i].texlayer_count; j++) {
            if(shading_descs[i].texcoord_dims[j] == 2) {
                flags |= RenderGroup::BUFFER_TEXCOORD0_MASK << 2 * j;
            }
        }
        int stride = __builtin_popcount(flags);
        GLfloat *data = new GLfloat[face_count[i] * 3 * stride];
        GLfloat *head = data;
        for(unsigned int j = 0; j < faces.size(); j++) {
            if(faces[j].shading_id == i) {
                for(int k = 0; k < 3; k++) {
                    memcpy(head, &positions[faces[j].corners[k].position], sizeof(GLfloat) * 3);
                    head += 3;
                    if(flags & RenderGroup::BUFFER_NORMAL_MASK) {
                        memcpy(head, &normals[faces[j].corners[k].normal], sizeof(GLfloat) * 3);
                        head += 3;
                    }
                    if(flags & RenderGroup::BUFFER_DIFFUSE_MASK) {
                        memcpy(head, &diffuse_colors[faces[j].corners[k].diffuse], sizeof(GLfloat) * 4);
                        head += 4;
                    }
                    if(flags & RenderGroup::BUFFER_SPECULAR_MASK) {
                        memcpy(head, &specular_colors[faces[j].corners[k].specular], sizeof(GLfloat) * 4);
                        head += 4;
                    }
                    for(int l = 0; l < 8; l++) {
                        if(flags & (RenderGroup::BUFFER_TEXCOORD0_MASK << (2 * l))) {
                            memcpy(head, &texcoords[faces[j].corners[k].texcoord[l]], sizeof(GLfloat) * 2);
                            head += 2;
                        }
                    }
                }
            }
        }
        group->load(i, data, flags, face_count[i] * 3);
        delete[] data;
    }
    return group;
}

}
