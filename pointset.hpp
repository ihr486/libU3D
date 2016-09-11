#pragma once

#include "bitstream.hpp"
#include "types.hpp"
#include "clod_common.hpp"

namespace U3D
{

class PointSet : private CLOD_Object, public Modifier
{
    struct Point
    {
        uint32_t shading_id;
        uint32_t position, normal, texcoord[8];
        uint32_t diffuse, specular;
    };
    std::vector<Point> points;
    std::vector<Vector3f> positions, normals;
    std::vector<Color4f> diffuse_colors, specular_colors;
    std::vector<TexCoord4f> texcoords;
    class PointIndexer
    {
        std::vector<uint32_t> positions;
    public:
        uint32_t get_point(uint32_t position_index) {
            return positions[position_index];
        }
        void add_point(const Point& point) {
        }
    };
    PointIndexer indexer;

    uint32_t last_diffuse, last_specular, last_texcoord;
public:
    PointSet(BitStreamReader& reader);
    void update_resolution(BitStreamReader& reader);
private:
    unsigned int get_texlayer_count(uint32_t shading_id) {
        return shading_descs[shading_id].texcoord_dims.size();
    }
};

}
