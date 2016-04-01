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

class U3D
{
    BinaryReader reader;
public:
    U3D(const std::string& filename)
        : reader(filename)
    {
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
