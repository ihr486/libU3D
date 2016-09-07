#pragma once

#include <cstdio>
#include <iostream>
#include <fstream>
#include <array>
#include <vector>

#include "types.hpp"

namespace U3D
{

class BitStreamReader
{
    static constexpr uint32_t StaticFull = 1024;

    class DynamicContext
    {
        std::vector<uint16_t> symbol_count;
        uint16_t total_symbol_count;
    public:
        enum {
            cShading = 1, cDiffuseCount, cDiffuseColorSign, cColorDiffR, cColorDiffG, cColorDiffB
        };
        DynamicContext() : symbol_count(256, 0), total_symbol_count(1)
        {
            symbol_count[0] = 1;
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
        /*uint32_t get_cumulative_symbol_frequency(uint32_t symbol) const
        {
        }*/
        uint32_t get_total_symbol_frequency() const
        {
            return total_symbol_count;
        }
        std::pair<uint32_t, uint32_t> get_symbol_from_frequency(uint32_t frequency) const
        {
            uint32_t cum_freq = 0;
            uint32_t symbol = 0;
            for(; symbol < symbol_count.size(); symbol++) {
                cum_freq += symbol_count[symbol];
                if(cum_freq > frequency) {
                    break;
                }
            }
            return std::make_pair(symbol, cum_freq);
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
    std::array<DynamicContext, StaticFull> dynamic_contexts;
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
        /*if(data_buffer.size() < 4) data_buffer.resize(4);
        ifs.read(reinterpret_cast<char *>(data_buffer.data()), 12);
        bit_position = 0;
        type = read<uint32_t>();
        uint32_t data_size = (read<uint32_t>() + 3) / 4;
        uint32_t metadata_size = (read<uint32_t>() + 3) / 4;
        std::fprintf(stderr, "Block opened: %08X-%u/%u\n", type, data_size, metadata_size);*/
    template<typename T> typename std::enable_if<std::is_pod<T>::value, T>::type read()
    {
        T ret;
        uint8_t *p = reinterpret_cast<uint8_t *>(&ret);
        for(unsigned int i = 0; i < sizeof(T); i++) {
            p[i] = read_byte();
        }
        return ret;
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
        return bit_reverse_table[read_static_symbol(256) - 1];
    }
    class ContextAdapter
    {
        BitStreamReader& reader;
        uint32_t context;
    public:
        ContextAdapter(BitStreamReader& reader, uint32_t context) : reader(reader), context(context) {}
        template<typename T> typename std::enable_if<std::is_pod<T>::value, T>::type read()
        {
            T ret;
            if(context < 0x3FFF) {

            }
            return ret;
        }
        template<typename T> ContextAdapter& operator>>(T& val)
        {
            val = read<T>();
            return *this;
        }
    };

    ContextAdapter operator[](uint32_t context)
    {
        return ContextAdapter(*this, context);
    }
};


}
