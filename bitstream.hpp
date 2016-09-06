#pragma once

#include <cstdio>
#include <iostream>
#include <fstream>

#include "types.hpp"

class BitStreamReader
{
    std::ifstream ifs;
    size_t byte_position;
public:
    BitStreamReader(const std::string& filename)
    {
        ifs.open(filename, std::ios::in);
        if(!ifs.is_open()) {
            std::fprintf(stderr, "Failed to open: %s.\n", filename.c_str());
        }
        byte_position = 0;
    }
    template<typename T> T read()
    {
        T ret;

        ifs.read(reinterpret_cast<char *>(&ret), sizeof(T));
        byte_position += sizeof(T);

        return ret;
    }
    template<typename T> BitStreamReader& operator>>(T& val)
    {
        ifs.read(reinterpret_cast<char *>(&val), sizeof(T));
        byte_position += sizeof(T);
        return *this;
    }
    BitStreamReader& operator>>(std::string& val)
    {
        uint16_t size = read<uint16_t>();
        for(unsigned int i = 0; i < size; i++) {
            val.push_back(ifs.get());
        }
        byte_position += size;
        return *this;
    }
    std::string read_str()
    {
        std::string ret;
        *this >> ret;
        return ret;
    }
    Vector3f read_vector3f()
    {
        return Vector3f{read<float>(), read<float>(), read<float>()};
    }
    Color3f read_color3f()
    {
        return Color3f{read<float>(), read<float>(), read<float>()};
    }
    Vector2f read_vector2f()
    {
        return Vector2f{read<float>(), read<float>()};
    }
    Matrix4f read_matrix4f()
    {
        Matrix4f ret;
        for(int x = 0; x < 4; x++) {
            for(int y = 0; y < 4; y++) {
                ret.m[y][x] = read<float>();
            }
        }
        return ret;
    }
    Quaternion4f read_quaternion4f()
    {
        return Quaternion4f{read<float>(), read<float>(), read<float>(), read<float>()};
    }
    void align_to_word()
    {
        if(byte_position % 4) {
            ifs.ignore(4 - (byte_position % 4));
            byte_position += 4 - (byte_position % 4);
        }
    }
    size_t get_position() const { return byte_position; }
    void skip(size_t n) {
        if(n > 0) {
            ifs.ignore(n);
            byte_position += n;
        }
    }
};

