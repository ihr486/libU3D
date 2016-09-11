#pragma once

#include "bitstream.hpp"
#include "types.hpp"
#include "clod_common.hpp"

namespace U3D
{

class PointSet : private CLOD_Object, public Modifier
{
    class PointIndexer
    {
    };
    PointIndexer indexer;
public:
    PointSet(BitStreamReader& reader);
    void update_resolution(BitStreamReader& reader);
};

}
