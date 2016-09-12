#pragma once

#include <cstdio>
#include <iostream>
#include <fstream>
#include <vector>

#include "types.hpp"

namespace U3D
{

enum ContextEnum {
    cZero = 0, cShading, cDiffuseCount, cDiffuseColorSign, cColorDiffR, cColorDiffG, cColorDiffB, cColorDiffA, cSpecularCount,
    cSpecularColorSign, cTexCoordCount, cTexCoordSign, cTexCDiffU, cTexCDiffV, cTexCDiffS, cTexCDiffT, cFaceCnt, cFaceOrnt,
    cThrdPosType, cLocal3rdPos, cStayMove0, cStayMove1, cStayMove2, cStayMove3, cStayMove4, cDiffuseKeepChange, cDiffuseChangeType,
    cDiffuseChangeIndexNew, cDiffuseChangeIndexLocal,
    cDiffuseChangeIndexGlobal, cSpecularKeepChange, cSpecularChangeType, cSpecularChangeIndexNew, cSpecularChangeIndexLocal,
    cSpecularChangeIndexGlobal, cTCKeepChange, cTCChangeType, cTCChangeIndexNew, cTCChangeIndexLocal, cTCChangeIndexGlobal,
    cColorDup, cColorIndexType, cColorIndexLocal, cColorIndexGlobal, cTexCDup, cTexCIndexType, cTextureIndexLocal,
    cTextureIndexGlobal, cPosDiffSign, cPosDiffX, cPosDiffY, cPosDiffZ, cNormalCnt, cDiffNormalSign, cDiffNormalX, cDiffNormalY,
    cDiffNormalZ, cNormalIdx, cPointCnt, cDiffDup, cSpecDup, cLineCnt, NumContexts
};

class BitStreamReader
{
    class DynamicContext
    {
        std::vector<uint16_t> symbol_count;
        uint16_t total_symbol_count;
    public:
        DynamicContext() : symbol_count(256, 0), total_symbol_count(1)
        {
            symbol_count[0] = 1;
        }
        void reset()
        {
            symbol_count.resize(256);
            std::fill(symbol_count.begin(), symbol_count.end(), 0);
            symbol_count[0] = 1;
            total_symbol_count = 1;
        }
        void add_symbol(uint32_t symbol)
        {
            if(symbol <= 0xFFFF) {
                if(symbol >= symbol_count.size()) {
                    symbol_count.resize(symbol + 1, 0);
                }
                if(total_symbol_count >= 0x1FFF) {
                    total_symbol_count = 1;
                    for(size_t i = 0; i < symbol_count.size(); i++) {
                        symbol_count[i] >>= 1;
                        total_symbol_count += symbol_count[i];
                    }
                    symbol_count[0]++;
                }
                symbol_count[symbol]++;
                total_symbol_count++;
            }
        }
        uint32_t get_symbol_frequency(uint32_t symbol) const
        {
            if(symbol >= symbol_count.size()) {
                return 0;
            } else {
                return symbol_count[symbol];
            }
        }
        uint32_t get_total_symbol_frequency() const
        {
            return total_symbol_count;
        }
        uint32_t get_symbol_from_frequency(uint32_t frequency, uint32_t *cum_freq) const
        {
            uint32_t symbol = 0, cum_freq_counter = 0;
            for(; symbol < symbol_count.size(); symbol++) {
                if(cum_freq_counter + symbol_count[symbol] > frequency) {
                    break;
                }
                cum_freq_counter += symbol_count[symbol];
            }
            *cum_freq = cum_freq_counter;
            return symbol;
        }
    };
public:
    class SubBlock
    {
        BitStreamReader& reader;
        uint32_t type;
        uint32_t data_size, metadata_size;
        size_t origin;
    public:
        SubBlock(BitStreamReader& reader) : reader(reader)
        {
            reader >> type >> data_size >> metadata_size;
            origin = reader.bit_position;
        }
        uint32_t get_type() const { return type; }
        ~SubBlock()
        {
            reader.bit_position = origin + ((data_size + 3) / 4 + (metadata_size + 3) / 4) * 32;
        }
    };
private:
    std::ifstream ifs;
    size_t bit_position;
    uint32_t high, low, underflow, type;
    std::vector<uint32_t> data_buffer, metadata_buffer;
    static const uint8_t bit_reverse_table[256];
    DynamicContext dynamic_contexts[NumContexts];
private:
    uint32_t read_word_direct()
    {
        uint32_t ret;
        ifs.read(reinterpret_cast<char *>(&ret), 4);
        return ret;
    }
public:
    BitStreamReader(const std::string& filename);
    bool open_block();
    template<typename T> T read()
    {
        T ret;
        uint8_t *p = reinterpret_cast<uint8_t *>(&ret);
        for(unsigned int i = 0; i < sizeof(T); i++) {
            p[i] = read_byte();
        }
        return ret;
    }
    void reset()
    {
        for(int i = 0; i < NumContexts; i++) {
            dynamic_contexts[i].reset();
        }
        high = 0xFFFF, low = 0, underflow = 0;
    }
    template<typename T> BitStreamReader& operator>>(T& val)
    {
        val = read<T>();
        return *this;
    }
    BitStreamReader& operator>>(std::string& val)
    {
        uint16_t size = read<uint16_t>();
        for(unsigned int i = 0; i < size; i++) {
            val.push_back(read<char>());
        }
        return *this;
    }
    uint32_t read_bits(unsigned int n)
    {
        uint64_t buffer = ((uint64_t)data_buffer[bit_position / 32 + 1] << 32) | data_buffer[bit_position / 32];
        uint32_t ret = (buffer >> (bit_position % 32)) & (((uint64_t)1 << n) - 1);
        bit_position += n;
        return ret;
    }
    uint32_t read_bit()
    {
        uint32_t ret = (data_buffer[bit_position / 32] >> (bit_position % 32)) & 1;
        bit_position++;
        return ret;
    }
    std::string read_str()
    {
        std::string ret;
        *this >> ret;
        return ret;
    }
    void align_to_word()
    {
        bit_position = ((bit_position + 31) / 32) * 32;
    }
    uint32_t get_type() const { return type; }
    uint32_t read_static_symbol(uint32_t context);
    uint32_t read_dynamic_symbol(uint32_t context);
    uint32_t read_byte()
    {
        uint32_t symbol = read_static_symbol(256);
        return bit_reverse_table[symbol - 1];
    }
    class ContextAdapter
    {
        BitStreamReader& reader;
        uint32_t context;
    public:
        ContextAdapter(BitStreamReader& reader, uint32_t context) : reader(reader), context(context) {}
        template<typename T> T read()
        {
            if(context < 0x3FFF) {
                uint32_t symbol = reader.read_static_symbol(context);
                if(symbol == 0) {
                    return reader.read<T>();
                } else {
                    return static_cast<T>(symbol - 1);
                }
            } else if(context == 0x3FFF) {
                return reader.read<T>();
            } else {
                uint32_t symbol = reader.read_dynamic_symbol(context - 0x4000);
                if(symbol == 0) {
                    T value = reader.read<T>();
                    reader.dynamic_contexts[context - 0x4000].add_symbol(static_cast<uint32_t>(value) + 1);
                    return value;
                } else {
                    return static_cast<T>(symbol - 1);
                }
            }
        }
        template<typename T> ContextAdapter& operator>>(T& val)
        {
            val = read<T>();
            return *this;
        }
    };
    ContextAdapter operator[](uint32_t context)
    {
        if(context < 0x3FFF) {
            return ContextAdapter(*this, context);
        } else {
            return ContextAdapter(*this, 0x3FFF);
        }
    }
    ContextAdapter operator[](ContextEnum context)
    {
        return ContextAdapter(*this, static_cast<uint32_t>(context) + 0x4000);
    }
};

}
