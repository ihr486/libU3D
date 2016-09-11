#include "pointset.hpp"

namespace U3D
{

PointSet::PointSet(BitStreamReader& reader) : CLOD_Object(reader)
{
}

void PointSet::update_resolution(BitStreamReader& reader)
{
    reader.read<uint32_t>();    //Chain index is always zero.
    uint32_t start, end;
    reader >> start >> end;
    for(unsigned int resolution = start; resolution < end; resolution++) {
        uint32_t split_position;
        if(resolution == 0) {
            split_position = reader[cZero].read<uint32_t>();
        } else {
            split_position = reader[resolution].read<uint32_t>();
        }
        /*Vector3f new_position = positions[split_position];
        uint8_t pos_sign = reader[cPosDiffSign].read<uint8_t>();
        uint32_t pos_X = reader[cPosDiffX].read<uint32_t>();
        uint32_t pos_Y = reader[cPosDiffY].read<uint32_t>();
        uint32_t pos_Z = reader[cPosDiffZ].read<uint32_t>();
        new_position.x += inverse_quant(pos_sign & 0x1, pos_X, position_iq);
        new_position.y += inverse_quant(pos_sign & 0x2, pos_Y, position_iq);
        new_position.z += inverse_quant(pos_sign & 0x4, pos_Z, position_iq);
        uint32_t new_norm_count = reader[cNormalCnt].read<uint32_t>();*/
    }
}

}
