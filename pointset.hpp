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
public:
    PointSet(BitStreamReader& reader);
    void update_resolution(BitStreamReader& reader);
};

}
