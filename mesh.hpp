#pragma once

#include <cstdio>
#include <vector>
#include <string>

#include "types.hpp"
#include "bitstream.hpp"
#include "clod_common.hpp"

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
        static bool is_face_neighbors(const Face& face1, const Face& face2) {
            int count = 0;
            for(int i = 0; i < 3; i++) {
                for(int j = 0; j < 3; j++) {
                    if(face1.corners[i].position == face2.corners[j].position) {
                        count++;
                    }
                }
            }
            return count == 2;
        }
        void add_face(uint32_t index, const Face& face) {
            for(int i = 0; i < 3; i++) {
                positions[face.corners[i].position].push_back(index);
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
                if(shading_descs[faces[positions[position][i]].shading_id].texcoord_dims.size() > layer) {
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
};

}
