#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>

class BinaryReader
{
    std::ifstream ifs;
public:
    BinaryReader(const std::string& filename)
    {
        ifs.open(filename, std::ios::in);
        if(!ifs.is_open()) {
            std::fprintf(stderr, "Failed to open: %s.\n", filename.c_str());
        }
    }
    template<typename T> T read()
    {
        T ret;

        ifs.read(reinterpret_cast<char *>(&ret), sizeof(T));

        return ret;
    }
    std::string read_str()
    {
        std::string ret;

        uint16_t size = read<uint16_t>();

        for(int i = 0; i < size; i++) {
            ret.push_back(ifs.get());
        }

        return ret;
    }
};

class Block
{
    uint32_t data_size, metadata_size;
public:
    Block(BinaryReader &reader) {
        data_size = reader.read<uint32_t>();
        metadata_size = reader.read<uint32_t>();
    }
};

class FileHeaderBlock : public Block
{
    uint16_t major_version, minor_version;
    uint32_t profile_identifier;
    uint32_t declaration_size;
    uint64_t file_size;
    uint32_t character_encoding;
    double units_scaling_factor;
public:
    FileHeaderBlock(BinaryReader &reader) {
        major_version = reader.read<uint16_t>();
        minor_version = reader.read<uint16_t>();
        profile_identifier = reader.read<uint32_t>();
        declaration_size = reader.read<uint32_t>();
        file_size = reader.read<uint64_t>();
        character_encoding = reader.read<uint32_t>();
        if(profile_identifier & 0x8) {
            units_scaling_factor = reader.read<double>();
        } else {
            units_scaling_factor = 1;
        };
    }
};


class U3D
{
    BinaryReader reader;
public:
    U3D(const std::string& filename)
        : reader(filename)
    {
        while(true) {
            uint32_t type = reader.read<uint32_t>();

            switch(type) {
            case 0x00443355:
            }
        }
    }
};

int main(int argc, char * const argv[])
{
    std::printf("Universal 3D loader v0.1a\n");

    if(argc != 2) {
        std::fprintf(stderr, "Please specify an input file.\n");
        return 1;
    }

    U3D u3d(argv[1]);
    
    return 0;
}
