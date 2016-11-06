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

class CLOD_Mesh : private CLOD_Object, public ModelResource
{
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
        Corner& get_corner(uint32_t p) {
            return corners[2].position == p ? corners[2] : (corners[1].position == p ? corners[1] : corners[0]);
        }
    };
    struct NewFace
    {
        uint32_t shading_id;
        uint8_t ornt;
        Corner corners[3];
    };
    std::vector<Face> faces;
    //Resolution update status
    uint32_t cur_res;
    Corner last_corners[3];
    class FaceIndexer
    {
        std::vector<std::vector<uint32_t> > positions;
    public:
        void add_face(uint32_t index, const Face& face) {
            for(int i = 0; i < 3; i++) {
                std::vector<uint32_t>& cont = positions[face.corners[i].position];
                cont.push_back(index);
                //positions[face.corners[i].position].push_back(index);
                std::sort(cont.begin(), cont.end(), std::greater<uint32_t>());
                //greater_unique_sort(positions[face.corners[i].position]);
            }
        }
        void add_position() {
            positions.push_back(std::vector<uint32_t>());
        }
        void add_positions(size_t n) {
            positions.insert(positions.end(), n, std::vector<uint32_t>());
        }
        std::vector<uint32_t> list_inclusive_neighbors(const std::vector<Face>& faces, uint32_t position) {
            std::vector<uint32_t> neighbors;
            for(unsigned int i = 0; i < positions[position].size(); i++) {
                for(int j = 0; j < 3; j++) {
                    neighbors.push_back(faces[positions[position][i]].corners[j].position);
                }
            }
            greater_unique_sort(neighbors);
            return neighbors;
        }
        std::vector<uint32_t> list_faces(uint32_t position) {
            if(position >= positions.size()) {
                return std::vector<uint32_t>();
            } else {
                return positions[position];
            }
        }
        void move_position(uint32_t face, uint32_t position, uint32_t new_position) {
            positions[position].erase(std::remove(positions[position].begin(), positions[position].end(), face), positions[position].end());
            positions[new_position].push_back(face);
        }
        std::vector<uint32_t> list_diffuse_colors(std::vector<Face>& faces, uint32_t position) {
            std::vector<uint32_t> ret;
            for(uint32_t i = 0; i < positions[position].size(); i++) {
                Corner& corner = faces[positions[position][i]].get_corner(position);
                ret.push_back(corner.diffuse);
            }
            greater_unique_sort(ret);
            return ret;
        }
        std::vector<uint32_t> list_specular_colors(std::vector<Face>& faces, uint32_t position) {
            std::vector<uint32_t> ret;
            for(unsigned int i = 0; i < positions[position].size(); i++) {
                Corner& corner = faces[positions[position][i]].get_corner(position);
                ret.push_back(corner.specular);
            }
            greater_unique_sort(ret);
            return ret;
        }
        std::vector<uint32_t> list_texcoords(std::vector<Face>& faces, const std::vector<ShadingDesc>& shading_descs, uint32_t position, unsigned int layer) {
            std::fprintf(stderr, "Listing texcoords...\n");
            std::vector<uint32_t> ret;
            for(unsigned int i = 0; i < positions[position].size(); i++) {
                if(shading_descs[faces[positions[position][i]].shading_id].texlayer_count > layer) {
                    Corner& corner = faces[positions[position][i]].get_corner(position);
                    ret.push_back(corner.texcoord[layer]);
                }
            }
            greater_unique_sort(ret);
            std::fprintf(stderr, "Listing texcoords complete.\n");
            return ret;
        }
        static int check_edge(const Face& face, uint32_t pos1, uint32_t pos2) {
            if(face.corners[0].position == pos1) {
                if(face.corners[1].position == pos2) {
                    return +1;
                } else {
                    return face.corners[2].position == pos2 ? -1 : 0;
                }
            } else if(face.corners[1].position == pos1) {
                if(face.corners[0].position == pos2) {
                    return -1;
                } else {
                    return face.corners[2].position == pos2 ? +1 : 0;
                }
            } else if(face.corners[2].position == pos1) {
                if(face.corners[0].position == pos2) {
                    return +1;
                } else {
                    return face.corners[1].position == pos2 ? -1 : 0;
                }
            } else {
                return 0;
            }
        }
    };
    FaceIndexer indexer;
public:
    CLOD_Mesh() : cur_res(0) {}
    CLOD_Mesh(BitStreamReader& reader);
    void create_base_mesh(BitStreamReader& reader);
    void update_resolution(BitStreamReader& reader);
    void dump_author_mesh();
    RenderGroup *create_render_group();
};

}
