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
};

class LineSet : private CLOD_Object, public Modifier
{
    struct Line
    {
        uint32_t shading_id;
        uint32_t position, normal, texcoord[8];
        uint32_t diffuse, specular;
    };
    std::vector<Line> lines;
    class LineIndexer
    {
    public:
    };
    LineIndexer indexer;
public:
    LineSet(BitStreamReader& reader);
    void update_resolution(BitStreamReader& reader);
};

}
