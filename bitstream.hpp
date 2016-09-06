#pragma once

#include <cstdio>
#include <iostream>
#include <fstream>
#include <array>
#include <vector>

#include "types.hpp"

class Context
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

class BitStreamReader
{
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
    operator bool() const
    {
        return !ifs.eof();
    }
    uint32_t read_word_direct()
    {
        uint32_t ret;
        ifs.read(reinterpret_cast<char *>(&ret), 4);
        return ret;
    }
    void open_block()
    {
        type = read_word_direct();
        uint32_t data_size = (read_word_direct() + 3) / 4;
        uint32_t metadata_size = (read_word_direct() + 3) / 4;
        data_buffer.resize(data_size + 1);
        metadata_buffer.resize(metadata_size + 1);
        ifs.read(reinterpret_cast<char *>(data_buffer.data()), 4 * data_size);
        ifs.read(reinterpret_cast<char *>(metadata_buffer.data()), 4 * metadata_size);
        data_buffer[data_size] = 0;
        metadata_buffer[metadata_size] = 0;
        bit_position = 0;
    }
    template<typename T> typename std::enable_if<sizeof(T) <= 4 && std::is_integral<T>::value, T>::type read()
    {
        uint64_t buffer = ((uint64_t)data_buffer[bit_position / 32 + 1] << 32) | data_buffer[bit_position / 32];
        T ret = static_cast<T>((buffer >> (bit_position % 32)) & (((uint64_t)1 << (8 * sizeof(T))) - 1));
        bit_position += 8 * sizeof(T);
        return ret;
    }
    template<typename T> typename std::enable_if<sizeof(T) == 8 && std::is_integral<T>::value, T>::type read()
    {
        return static_cast<T>(read<uint32_t>() | ((uint64_t)read<uint32_t>() << 32));
    }
    template<typename T> typename std::enable_if<sizeof(T) % 4 == 0 && !std::is_integral<T>::value && std::is_pod<T>::value, T>::type read()
    {
        T ret;
        uint32_t *p = reinterpret_cast<uint32_t *>(&ret);
        for(unsigned int i = 0; i < sizeof(T) / 4; i++) {
            p[i] = read<uint32_t>();
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
    /*BitStreamReader& operator>>(Vector3f& vec)
    {
        vec.x = read<float>(), vec.y = read<float>(), vec.z = read<float>();
        return *this;
    }
    BitStreamReader& operator>>(Color3f& col)
    {
        col.r = read<float>(), col.g = read<float>(), col.b = read<float>();
        return *this;
    }
    BitStreamReader& operator>>(Vector2f& vec)
    {
        vec.u = read<float>(), vec.v = read<float>();
        return *this;
    }
    BitStreamReader& operator>>(Quaternion4f& q)
    {
        q.w = read<float>(), q.x = read<float>(), q.y = read<float>(), q.z = read<float>();
        return *this;
    }
    BitStreamReader& operator>>(Matrix4f& mat)
    {
        for(int x = 0; x < 4; x++) {
            for(int y = 0; y < 4; y++) {
                mat.m[y][x] = read<float>();
            }
        }
        return *this;
    }*/
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
};

