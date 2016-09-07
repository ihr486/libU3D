#pragma once

#include <cstdio>
#include <iostream>
#include <fstream>
#include <array>
#include <vector>

#include "types.hpp"

class BitStreamReader
{
    friend class DynamicContext;

    std::ifstream ifs;
    size_t bit_position;
    uint32_t high, low, underflow, type;
    std::vector<uint32_t> data_buffer, metadata_buffer;
public:
    BitStreamReader(const std::string& filename) : bit_position(0), high(0xFFFF), low(0), underflow(0)
    {
        ifs.open(filename, std::ios::in);
        if(!ifs.is_open()) {
            std::fprintf(stderr, "Failed to open: %s.\n", filename.c_str());
        }
    }
    uint32_t read_word_direct()
    {
        uint32_t ret;
        ifs.read(reinterpret_cast<char *>(&ret), 4);
        return ret;
    }
    bool open_block()
    {
        /*if(data_buffer.size() < 4) data_buffer.resize(4);
        ifs.read(reinterpret_cast<char *>(data_buffer.data()), 12);
        bit_position = 0;
        type = read<uint32_t>();
        uint32_t data_size = (read<uint32_t>() + 3) / 4;
        uint32_t metadata_size = (read<uint32_t>() + 3) / 4;
        std::fprintf(stderr, "Block opened: %08X-%u/%u\n", type, data_size, metadata_size);*/
        type = read_word_direct();
        if(ifs.eof()) return false;
        uint32_t data_size = (read_word_direct() + 3) / 4;
        uint32_t metadata_size = (read_word_direct() + 3) / 4;
        data_buffer.resize(data_size + 1);
        metadata_buffer.resize(metadata_size + 1);
        ifs.read(reinterpret_cast<char *>(data_buffer.data()), 4 * data_size);
        ifs.read(reinterpret_cast<char *>(metadata_buffer.data()), 4 * metadata_size);
        data_buffer[data_size] = 0;
        metadata_buffer[metadata_size] = 0;
        bit_position = 0;
        return true;
    }
    template<typename T> T read()
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
        /*uint64_t buffer = ((uint64_t)data_buffer[bit_position / 32 + 1] << 32) | data_buffer[bit_position / 32];
        uint32_t ret = (buffer >> (bit_position % 32)) & (((uint64_t)1 << n) - 1);
        bit_position += n;
        return ret;*/
        uint32_t ret = 0;
        for(unsigned int i = 0; i < n; i++) {
            ret = (ret << 1) | read_bit();
        }
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
    uint32_t read_static_symbol(uint32_t context)
    {
        size_t checkpoint = bit_position;
        uint32_t code = read_bit() << 15;
        bit_position += underflow;
        code |= read_bits(15);
        bit_position = checkpoint;
        
        uint32_t range = high + 1 - low;
        uint32_t cum_freq = (context * (1 + code - low) - 1) / range;
        uint32_t value = cum_freq + 1;

        high = low + range * (cum_freq + 1) / context - 1;
        low = low + range * cum_freq / context;

        unsigned int bit_count = 0;
        while((low & 0x8000) == (high & 0x8000)) {
            low = (0x7FFF & low) << 1;
            high = ((0x7FFF & high) << 1) | 1;
            bit_count++;
        }
        if(bit_count > 0) {
            bit_count += underflow;
            underflow = 0;
        }
        while((low & 0x4000) && !(high & 0x4000)) {
            low = (low & 0x3FFF) << 1;
            high = ((high & 0x3FFF) << 1) | 0x8001;
            underflow++;
        }
        bit_position += bit_count;
        return value;
    }
    uint32_t read_byte()
    {
        static const uint8_t bit_reverse_table[] = 
        {
            0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0, 
            0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8, 
            0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4, 
            0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC, 
            0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2, 
            0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
            0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6, 
            0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
            0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
            0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9, 
            0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
            0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
            0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3, 
            0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
            0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7, 
            0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
        };
        return bit_reverse_table[read_static_symbol(256) - 1];
    }
};

/*class DynamicContext
{
    std::vector<uint16_t> symbol_count, cumul_count;
public:
    enum {
        cShading = 1, cDiffuseCount, cDiffuseColorSign, cColorDiffR, cColorDiffG, cColorDiffB
    };
    void add_symbol(uint32_t symbol)
    {
    }
    uint32_t get_symbol_frequency(uint32_t symbol) const
    {
    }
    uint32_t get_cumulative_symbol_frequency(uint32_t symbol) const
    {
    }
    uint32_t get_total_symbol_frequency() const
    {
    }
    uint32_t get_symbol_from_frequency(uint32_t frequency) const
    {
    }
};

struct Context
{
    BitStreamReader& reader;
public:
};

Context BitStreamReader::operator[](uint32_t context)
{
    return Context{};
}*/
