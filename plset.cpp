#include "plset.hpp"

namespace U3D
{

PointSet::PointSet(BitStreamReader& reader) : CLOD_Object(false, reader)
{
    last_diffuse = 0, last_specular = 0;
    for(int i = 0; i < 8; i++) last_texcoord[i] = 0;
}

void PointSet::update_resolution(BitStreamReader& reader)
{
    reader.read<uint32_t>();    //Chain index is always zero.
    uint32_t start, end;
    reader >> start >> end;
    for(unsigned int resolution = start; resolution < end; resolution++) {
        uint32_t split_position, split_point;
        Vector3f pred_position;
        if(resolution == 0) {
            split_position = reader[cZero].read<uint32_t>();
        } else {
            split_position = reader[resolution].read<uint32_t>();
            pred_position = positions[split_position];
        }
        uint8_t pos_sign = reader[cPosDiffSign].read<uint8_t>();
        uint32_t pos_X = reader[cPosDiffX].read<uint32_t>();
        uint32_t pos_Y = reader[cPosDiffY].read<uint32_t>();
        uint32_t pos_Z = reader[cPosDiffZ].read<uint32_t>();
        positions.push_back(pred_position + Vector3f::dequantize(pos_sign, pos_X, pos_Y, pos_Z, position_iq));
        uint32_t new_normal_count = reader[cNormalCnt].read<uint32_t>();
        Vector3f pred_normal;
        split_point = split_position;
        if(resolution > 0) {
            pred_normal = normals[points[split_point].normal];
        }
        for(unsigned int i = 0; i < new_normal_count; i++) {
            uint8_t norm_sign = reader[cDiffNormalSign].read<uint8_t>();
            uint32_t norm_X = reader[cDiffNormalX].read<uint32_t>();
            uint32_t norm_Y = reader[cDiffNormalY].read<uint32_t>();
            uint32_t norm_Z = reader[cDiffNormalZ].read<uint32_t>();
            normals.push_back(pred_normal + Vector3f::dequantize(norm_sign, norm_X, norm_Y, norm_Z, normal_iq));
        }
        uint32_t new_point_count = reader[cPointCnt].read<uint32_t>();
        Color4f pred_diffuse, pred_specular;
        TexCoord4f pred_texcoord[8];
        if(resolution > 0) {
            pred_diffuse = diffuse_colors[points[split_point].diffuse];
            pred_specular = specular_colors[points[split_point].specular];
            for(unsigned int i = 0; i < get_texlayer_count(points[split_point].shading_id); i++) {
                pred_texcoord[i] = texcoords[points[split_point].texcoord[i]];
            }
        }
        for(unsigned int i = 0; i < new_point_count; i++) {
            Point new_point;
            new_point.shading_id = reader[cShading].read<uint32_t>();
            new_point.normal = normals.size() - new_normal_count + reader[cNormalIdx].read<uint32_t>();
            if(shading_descs[new_point.shading_id].attributes & 0x00000001) {
                uint8_t dup_flag = reader[cDiffDup].read<uint8_t>();
                if(!(dup_flag & 0x2)) {
                    uint8_t diffuse_sign = reader[cDiffuseColorSign].read<uint8_t>();
                    uint32_t diffuse_R = reader[cColorDiffR].read<uint32_t>();
                    uint32_t diffuse_G = reader[cColorDiffG].read<uint32_t>();
                    uint32_t diffuse_B = reader[cColorDiffB].read<uint32_t>();
                    uint32_t diffuse_A = reader[cColorDiffA].read<uint32_t>();
                    
                    new_point.diffuse = diffuse_colors.size();
                    diffuse_colors.push_back(pred_diffuse + Color4f::dequantize(diffuse_sign, diffuse_R, diffuse_G, diffuse_B, diffuse_A, diffuse_iq));
                } else {
                    new_point.diffuse = last_diffuse;
                }
                last_diffuse = new_point.diffuse;
            }
            if(shading_descs[new_point.shading_id].attributes & 0x00000002) {
                uint8_t dup_flag = reader[cSpecDup].read<uint8_t>();
                if(!(dup_flag & 0x2)) {
                    uint8_t specular_sign = reader[cSpecularColorSign].read<uint8_t>();
                    uint32_t specular_R = reader[cColorDiffR].read<uint32_t>();
                    uint32_t specular_G = reader[cColorDiffG].read<uint32_t>();
                    uint32_t specular_B = reader[cColorDiffB].read<uint32_t>();
                    uint32_t specular_A = reader[cColorDiffA].read<uint32_t>();
                    
                    new_point.specular = specular_colors.size();
                    specular_colors.push_back(pred_specular + Color4f::dequantize(specular_sign, specular_R, specular_G, specular_B, specular_A, specular_iq));
                } else {
                    new_point.specular = last_specular;
                }
                last_specular = new_point.specular;
            }
            for(unsigned int j = 0; j < get_texlayer_count(new_point.shading_id); j++) {
                uint8_t dup_flag = reader[cTexCDup].read<uint8_t>();
                if(!(dup_flag & 0x2)) {
                    uint8_t texcoord_sign = reader[cTexCoordSign].read<uint8_t>();
                    uint32_t texcoord_U = reader[cTexCDiffU].read<uint32_t>();
                    uint32_t texcoord_V = reader[cTexCDiffV].read<uint32_t>();
                    uint32_t texcoord_S = reader[cTexCDiffS].read<uint32_t>();
                    uint32_t texcoord_T = reader[cTexCDiffT].read<uint32_t>();

                    new_point.texcoord[j] = texcoords.size();
                    texcoords.push_back(pred_texcoord[j] + TexCoord4f::dequantize(texcoord_sign, texcoord_U, texcoord_V, texcoord_S, texcoord_T, texcoord_iq));
                } else {
                    new_point.texcoord[j] = last_texcoord[j];
                }
                last_texcoord[j] = new_point.texcoord[j];
            }
            points.push_back(new_point);
        }
    }
}

LineSet::LineSet(BitStreamReader& reader) : CLOD_Object(false, reader)
{
    last_diffuse = 0, last_specular = 0;
    for(int i = 0; i < 8; i++) last_texcoord[i] = 0;
}

void LineSet::update_resolution(BitStreamReader& reader)
{
    reader.read<uint32_t>();    //Chain index is always zero.
    uint32_t start, end;
    reader >> start >> end;
    for(unsigned int resolution = start; resolution < end; resolution++) {
        uint32_t split_position;
        Vector3f new_position;
        if(resolution == 0) {
            split_position = reader[cZero].read<uint32_t>();
        } else {
            split_position = reader[resolution].read<uint32_t>();
            new_position = positions[split_position];
        }
        std::vector<uint32_t> split_lines = indexer.list_lines(split_position);
        uint8_t pos_sign = reader[cPosDiffSign].read<uint8_t>();
        uint32_t pos_X = reader[cPosDiffX].read<uint32_t>();
        uint32_t pos_Y = reader[cPosDiffY].read<uint32_t>();
        uint32_t pos_Z = reader[cPosDiffZ].read<uint32_t>();
        new_position += Vector3f::dequantize(pos_sign, pos_X, pos_Y, pos_Z, position_iq);
        positions.push_back(new_position);
        indexer.add_position();
        uint32_t new_normal_count = reader[cNormalCnt].read<uint32_t>();
        Vector3f pred_normal;
        for(unsigned int i = 0; i < split_lines.size(); i++) {
            pred_normal += normals[lines[split_lines[i]].get_terminal(split_position).normal];
        }
        pred_normal = pred_normal.normalize();
        for(unsigned int i = 0; i < new_normal_count; i++) {
            uint8_t norm_sign = reader[cDiffNormalSign].read<uint8_t>();
            uint32_t norm_X = reader[cDiffNormalX].read<uint32_t>();
            uint32_t norm_Y = reader[cDiffNormalY].read<uint32_t>();
            uint32_t norm_Z = reader[cDiffNormalZ].read<uint32_t>();
            normals.push_back(pred_normal + Vector3f::dequantize(norm_sign, norm_X, norm_Y, norm_Z, normal_iq));
        }
        uint32_t new_line_count = reader[cLineCnt].read<uint32_t>();
        Line new_line;
        for(unsigned int i = 0; i < new_line_count; i++) {
            Color4f pred_diffuse, pred_specular;
            TexCoord4f pred_texcoord[8];
            new_line.shading_id = reader[cShading].read<uint32_t>();
            new_line.terminals[0].position = reader[positions.size() - 1].read<uint32_t>();
            new_line.terminals[1].position = positions.size() - 1;
            for(unsigned int j = 0; j < split_lines.size(); j++) {
                Terminal& terminal = lines[split_lines[j]].get_terminal(split_position);
                pred_diffuse += diffuse_colors[terminal.diffuse];
                pred_specular += specular_colors[terminal.specular];
                for(unsigned int k = 0; k < get_texlayer_count(lines[split_lines[j]].shading_id); k++) {
                    pred_texcoord[k] += texcoords[terminal.texcoord[k]];
                }
            }
            if(!split_lines.empty()) {
                pred_diffuse /= split_lines.size();
                pred_specular /= split_lines.size();
                for(unsigned int k = 0; k < 8; k++) {
                    pred_texcoord[k] /= split_lines.size();
                }
            }
            for(int j = 0; j < 2; j++) {
                new_line.terminals[j].normal = normals.size() - new_normal_count + reader[cNormalIdx].read<uint32_t>();
                if(shading_descs[new_line.shading_id].attributes & 0x00000001) {
                    uint8_t dup_flag = reader[cDiffDup].read<uint8_t>();
                    if(!(dup_flag & 0x2)) {
                        uint8_t diffuse_sign = reader[cDiffuseColorSign].read<uint8_t>();
                        uint32_t diffuse_R = reader[cColorDiffR].read<uint32_t>();
                        uint32_t diffuse_G = reader[cColorDiffG].read<uint32_t>();
                        uint32_t diffuse_B = reader[cColorDiffB].read<uint32_t>();
                        uint32_t diffuse_A = reader[cColorDiffA].read<uint32_t>();
                        
                        new_line.terminals[j].diffuse = diffuse_colors.size();
                        diffuse_colors.push_back(pred_diffuse + Color4f::dequantize(diffuse_sign, diffuse_R, diffuse_G, diffuse_B, diffuse_A, diffuse_iq));
                    } else {
                        new_line.terminals[j].diffuse = last_diffuse;
                    }
                    last_diffuse = new_line.terminals[j].diffuse;
                }
                if(shading_descs[new_line.shading_id].attributes & 0x00000002) {
                    uint8_t dup_flag = reader[cSpecDup].read<uint8_t>();
                    if(!(dup_flag & 0x2)) {
                        uint8_t specular_sign = reader[cSpecularColorSign].read<uint8_t>();
                        uint32_t specular_R = reader[cColorDiffR].read<uint32_t>();
                        uint32_t specular_G = reader[cColorDiffG].read<uint32_t>();
                        uint32_t specular_B = reader[cColorDiffB].read<uint32_t>();
                        uint32_t specular_A = reader[cColorDiffA].read<uint32_t>();
                        
                        new_line.terminals[j].specular = specular_colors.size();
                        specular_colors.push_back(pred_specular + Color4f::dequantize(specular_sign, specular_R, specular_G, specular_B, specular_A, specular_iq));
                    } else {
                        new_line.terminals[j].specular = last_specular;
                    }
                    last_specular = new_line.terminals[j].specular;
                }
                for(unsigned int k = 0; k < get_texlayer_count(new_line.shading_id); k++) {
                    uint8_t dup_flag = reader[cTexCDup].read<uint8_t>();
                    if(!(dup_flag & 0x2)) {
                        uint8_t texcoord_sign = reader[cTexCoordSign].read<uint8_t>();
                        uint32_t texcoord_U = reader[cTexCDiffU].read<uint32_t>();
                        uint32_t texcoord_V = reader[cTexCDiffV].read<uint32_t>();
                        uint32_t texcoord_S = reader[cTexCDiffS].read<uint32_t>();
                        uint32_t texcoord_T = reader[cTexCDiffT].read<uint32_t>();

                        new_line.terminals[j].texcoord[k] = texcoords.size();
                        texcoords.push_back(pred_texcoord[k] + TexCoord4f::dequantize(texcoord_sign, texcoord_U, texcoord_V, texcoord_S, texcoord_T, texcoord_iq));
                    } else {
                        new_line.terminals[j].texcoord[k] = last_texcoord[k];
                    }
                    last_texcoord[k] = new_line.terminals[j].texcoord[k];
                }
            }
            lines.push_back(new_line);
            indexer.set_line(positions.size() - 1, lines.size() - 1);
        }
    }
}

RenderGroup *PointSet::create_render_group()
{
    RenderGroup *rg = new RenderGroup();
    return rg;
}

RenderGroup *LineSet::create_render_group()
{
    RenderGroup *rg = new RenderGroup();
    return rg;
}

}
