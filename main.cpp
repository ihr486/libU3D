#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <cmath>
#include <vector>
#include <unordered_map>

#include "types.hpp"
#include "bitstream.hpp"

class Block
{
    BitStreamReader& reader;
    uint32_t data_size, metadata_size;
    size_t position;
private:
    template<typename T, typename U> static T alignto(T a, U n) { return ((a + n - 1) / n) * n; }
public:
    Block(BitStreamReader& reader) : reader(reader) {
        reader >> data_size >> metadata_size;
        position = reader.get_position();
    }
    void jump_to_metadata() {
        reader.skip(alignto(data_size, 4) + position - reader.get_position());
    }
    ~Block() {
        ssize_t n = alignto(data_size, 4) + alignto(metadata_size, 4) + position - reader.get_position();
        if(n < 0) {
            std::fprintf(stderr, "Overrun: %ld.\n", -n);
        } else {
            reader.skip(n);
        }
    }
};

class Modifier
{
public:
    Modifier() {}
    ~Modifier() {}
};

class FileHeader
{
    uint16_t major_version, minor_version;
    uint32_t profile_identifier;
    uint32_t declaration_size;
    uint64_t file_size;
    uint32_t character_encoding;
    double units_scaling_factor;
public:
    FileHeader(BitStreamReader& reader) {
        Block block(reader);
        reader >> major_version >> minor_version >> profile_identifier >> declaration_size >> file_size >> character_encoding;
        units_scaling_factor = (profile_identifier & 0x8) ? reader.read<double>() : 1;
        std::fprintf(stderr, "FileHeaderBlock created.\n");
    }
};

class PriorityManager
{
    uint32_t priority;
public:
    PriorityManager() : priority(0) {}
    uint32_t read_update_block(BitStreamReader& reader) {
        Block block(reader);
        reader >> priority;
        std::fprintf(stderr, "New priority : %u.\n", priority);
        return priority;
    }
};

class Node
{
protected:
    struct Parent {
        std::string name;
        Matrix4f transform;
    };
    std::string name;
    std::vector<Parent> parents;
public:
    void read(BitStreamReader& reader)
    {
        reader >> name;
        uint32_t parent_count = reader.read<uint32_t>();
        parents.resize(parent_count);
        for(unsigned int i = 0; i < parent_count; i++) {
            reader >> parents[i].name >> parents[i].transform;
        }
    }
};

class Group : public Node, public Modifier
{
public:
    Group(BitStreamReader& reader)
    {
        Block block(reader);
        Node::read(reader);
        std::fprintf(stderr, "Group node \"%s\"\n", name.c_str());
    }
};

class View : public Node, public Modifier
{
    std::string resource_name;
    uint32_t attributes;
    float near_clipping, far_clipping;
    float projection, ortho_height;
    Vector3f proj_vector;
    float port_x, port_y, port_w, port_h;
    struct Backdrop
    {
        std::string texture_name;
        float blend, rotation, location_x, location_y;
        int32_t reg_x, reg_y;
        float scale_x, scale_y;
    };
    std::vector<Backdrop> backdrops, overlays;
public:
    View(BitStreamReader& reader)
    {
        Block block(reader);
        Node::read(reader);
        reader >> resource_name >> attributes >> near_clipping >> far_clipping;
        switch(attributes & 0x6) {
        case 0: //Three-point perspective projection
            reader >> projection;
            break;
        case 2: //Orthographic projection
            reader >> ortho_height;
            break;
        case 4: //One-point perspective projection
        case 6: //Two-point perspective projection
            reader >> proj_vector;
            break;
        }
        reader >> port_w >> port_h >> port_x >> port_y;
        uint32_t backdrop_count = reader.read<uint32_t>();
        backdrops.resize(backdrop_count);
        for(unsigned int i = 0; i < backdrop_count; i++) {
            reader >> backdrops[i].texture_name >> backdrops[i].blend >> backdrops[i].location_x >> backdrops[i].location_y;
            reader >> backdrops[i].reg_x >> backdrops[i].reg_y >> backdrops[i].scale_x >> backdrops[i].scale_y;
        }
        uint32_t overlay_count = reader.read<uint32_t>();
        overlays.resize(overlay_count);
        for(unsigned int i = 0; i < overlay_count; i++) {
            reader >> backdrops[i].texture_name >> backdrops[i].blend >> backdrops[i].location_x >> backdrops[i].location_y;
            reader >> backdrops[i].reg_x >> backdrops[i].reg_y >> backdrops[i].scale_x >> backdrops[i].scale_y;
        }

        std::fprintf(stderr, "\tView \"%s\" -> \"%s\"\n", name.c_str(), resource_name.c_str());
    }
};

class Model : public Node, public Modifier
{
    std::string resource_name;
    uint32_t visibility;
public:
    Model(BitStreamReader& reader)
    {
        Block block(reader);
        Node::read(reader);
        reader >> resource_name >> visibility;

        std::fprintf(stderr, "\tModel \"%s\" -> \"%s\"\n", name.c_str(), resource_name.c_str());
    }
};

class Light : public Node, public Modifier
{
    std::string resource_name;
public:
    Light(BitStreamReader& reader)
    {
        Block block(reader);
        Node::read(reader);
        reader >> resource_name;

        std::fprintf(stderr, "\tLight \"%s\" -> \"%s\"\n", name.c_str(), resource_name.c_str());
    }
};

class Shading : public Modifier
{
    std::string name;
    uint32_t chain_index, attributes;
    std::vector<std::vector<std::string>> shader_names;
public:
    Shading(BitStreamReader& reader)
    {
        Block block(reader);
        reader >> name >> chain_index >> attributes;
        uint32_t list_count = reader.read<uint32_t>();
        shader_names.resize(list_count);
        for(unsigned int i = 0; i < list_count; i++) {
            uint32_t shader_count = reader.read<uint32_t>();
            shader_names[i].resize(shader_count);
            for(unsigned int j = 0; j < shader_count; j++) {
                reader >> shader_names[i][j];
            }
        }

        std::fprintf(stderr, "\tShading \"%s\"\n", name.c_str());
    }
};

class CLOD_Mesh : public Modifier
{
    std::string name;
    uint32_t chain_index;
    struct ShadingDesc {
        uint32_t attributes;
        std::vector<uint32_t> texcoord_dims;
    };
    struct MaxMeshDesc {
        uint32_t attributes;
        uint32_t face_count, position_count, normal_count;
        uint32_t diffuse_count, specular_count, texcoord_count;
        std::vector<ShadingDesc> shading_descs;
    };
    MaxMeshDesc maxmesh;
    uint32_t min_res, max_res;
    uint32_t position_quality, normal_quality, texcoord_quality;
    float position_iq, normal_iq, texcoord_iq, diffuse_iq, specular_iq;
    float normal_crease, normal_update, normal_tolerance;
    struct Bone {
        std::string name, parent_name;
        uint32_t attributes;
        float length;
        Vector3f displacement;
        Quaternion4f orientation;
        uint32_t link_count;
        float link_length;
        Vector2f start_joint_center, start_joint_scale;
        Vector2f end_joint_center, end_joint_scale;
    };
    std::vector<Bone> skeleton;
public:
    CLOD_Mesh(BitStreamReader& reader)
    {
        Block block(reader);
        reader >> name >> chain_index;
        reader >> maxmesh.attributes >> maxmesh.face_count >> maxmesh.position_count >> maxmesh.normal_count;
        reader >> maxmesh.diffuse_count >> maxmesh.specular_count >> maxmesh.texcoord_count;
        uint32_t shading_count = reader.read<uint32_t>();
        maxmesh.shading_descs.resize(shading_count);
        for(unsigned int i = 0; i < shading_count; i++) {
            maxmesh.shading_descs[i].attributes = reader.read<uint32_t>();
            uint32_t texlayer_count = reader.read<uint32_t>();
            maxmesh.shading_descs[i].texcoord_dims.resize(texlayer_count);
            for(unsigned int j = 0; j < texlayer_count; j++) {
                maxmesh.shading_descs[i].texcoord_dims[j] = reader.read<uint32_t>();
            }
            reader.read<uint32_t>();
        }
        reader >> min_res >> max_res;
        reader >> position_quality >> normal_quality >> texcoord_quality;
        reader >> position_iq >> normal_iq >> texcoord_iq >> diffuse_iq >> specular_iq;
        reader >> normal_crease >> normal_update >> normal_tolerance;
        uint32_t bone_count = reader.read<uint32_t>();
        skeleton.resize(bone_count);
        for(unsigned int i = 0; i < bone_count; i++) {
            reader >> skeleton[i].name >> skeleton[i].parent_name >> skeleton[i].attributes;
            reader >> skeleton[i].length >> skeleton[i].displacement >> skeleton[i].orientation;
            if(skeleton[i].attributes & 0x00000001) {
                reader >> skeleton[i].link_count >> skeleton[i].link_length;
            }
            if(skeleton[i].attributes & 0x00000002) {
                reader >> skeleton[i].start_joint_center >> skeleton[i].start_joint_scale;
                reader >> skeleton[i].end_joint_center >> skeleton[i].end_joint_scale;
            }
            for(int j = 0; j < 6; j++) {
                reader.read<float>();   //Skip past the rotation constraints
            }
        }
        std::fprintf(stderr, "\tCLOD Mesh \"%s\"\n", name.c_str());
    }
};

class ModifierChain
{
    std::string name;
    uint32_t type, attribute;
    Vector3f bsphere;
    float bsphere_radius;
    Vector3f aabb_min, aabb_max;
    uint32_t modifier_count;
    std::list<Modifier *> modifiers;
public:
    ModifierChain(BitStreamReader& reader) {
        Block block(reader);
        reader >> name >> type >> attribute;
        if(attribute & 0x00000001) {
            reader >> bsphere >> bsphere_radius;
        } else if(attribute & 0x00000002) {
            reader >> aabb_min >> aabb_max;
        }
        reader.align_to_word();
        modifier_count = reader.read<uint32_t>();

        std::fprintf(stderr, "Modifier Chain: %s [%u blocks]\n", name.c_str(), modifier_count);

        for(unsigned int i = 0; i < modifier_count; i++) {
            uint32_t type = reader.read<uint32_t>();
            switch(type) {
            case 0xFFFFFF21:    //Group Node Block
                modifiers.push_back(new Group(reader));
                break;
            case 0xFFFFFF22:    //Model Node Block
                modifiers.push_back(new Model(reader));
                break;
            case 0xFFFFFF23:    //Light Node Block
                modifiers.push_back(new Light(reader));
                break;
            case 0xFFFFFF24:    //View Node Block
                modifiers.push_back(new View(reader));
                break;
            case 0xFFFFFF31:    //CLOD Mesh Generator Declaration
                modifiers.push_back(new CLOD_Mesh(reader));
                break;
            case 0xFFFFFF36:    //Point Set Declaration
                break;
            case 0xFFFFFF41:    //2D Glyph Modifier Block
                break;
            case 0xFFFFFF42:    //Subdivision Modifier Block
                break;
            case 0xFFFFFF43:    //Animation Modifier Block
                break;
            case 0xFFFFFF44:    //Bone Weight Modifier Block
                break;
            case 0xFFFFFF45:    //Shading Modifier Block
                modifiers.push_back(new Shading(reader));
                break;
            case 0xFFFFFF46:    //CLOD Modifier Block
                break;
            default:
                std::fprintf(stderr, "Unsupported modifier type: 0x%08X.\n", type);
                return;
            }
        }
    }
};

class Resource
{
};

class LitTextureShader : public Resource
{
    std::string name;
    uint32_t attributes;
    float alpha_reference;
    uint32_t alpha_function, blend_function;
    uint32_t render_pass_flags;
    uint32_t shader_channels, alpha_texture_channels;
    std::string material_name;
    struct TextureInfo
    {
        std::string name;
        float intensity;
        uint8_t blend_function;
        uint8_t blend_source;
        float blend_constant;
        uint8_t mode;
        Matrix4f transform;
        Matrix4f wrap_transform;
        uint8_t repeat;
    };
    TextureInfo texinfos[8];
public:
    LitTextureShader(BitStreamReader& reader)
    {
        Block block(reader);
        reader >> name >> attributes >> alpha_reference >> alpha_function >> blend_function;
        reader >> render_pass_flags >> shader_channels >> alpha_texture_channels >> material_name;
        for(unsigned int i = 0; i < 8; i++) {
            if(shader_channels & (1 << i)) {
                reader >> texinfos[i].name >> texinfos[i].intensity >> texinfos[i].blend_function >> texinfos[i].blend_source;
                reader >> texinfos[i].blend_constant >> texinfos[i].mode >> texinfos[i].transform >> texinfos[i].wrap_transform;
                reader >> texinfos[i].repeat;
            }
        }
        std::fprintf(stderr, "Lit Texture Shader Resource \"%s\"\n", name.c_str());
    }
};

class Material : public Resource
{
    std::string name;
    uint32_t attributes;
    Color3f ambient, diffuse, specular, emissive;
    float reflectivity, opacity;
public:
    Material(BitStreamReader& reader)
    {
        Block block(reader);
        reader >> name >> attributes >> ambient >> diffuse >> specular >> emissive >> reflectivity >> opacity;
        std::fprintf(stderr, "Material \"%s\"\n", name.c_str());
    }
};

class LightResource : public Resource
{
    std::string name;
    uint32_t attributes;
    uint8_t type;
    Color3f color;
    float att_constant, att_linear, att_quadratic;
    float spot_angle, intensity;
public:
    LightResource(BitStreamReader& reader)
    {
        Block block(reader);
        reader >> name >> attributes >> type >> color;
        reader.read<float>();
        reader >> att_constant >> att_linear >> att_quadratic >> spot_angle >> intensity;
        std::fprintf(stderr, "Light Resource \"%s\"\n", name.c_str());
    }
};

class ViewResource : public Resource
{
    std::string name;
    struct Pass
    {
        std::string root_node_name;
        uint32_t render_attributes;
        uint32_t fog_mode;
        Color3f fog_color;
        float fog_alpha;
        float fog_near, fog_far;
    };
    std::vector<Pass> passes;
public:
    ViewResource(BitStreamReader& reader)
    {
        Block block(reader);
        name = reader.read_str();
        uint32_t pass_count = reader.read<uint32_t>();
        passes.resize(pass_count);
        for(unsigned int i = 0; i < pass_count; i++) {
            reader >> passes[i].root_node_name >> passes[i].render_attributes;
            reader >> passes[i].fog_mode >> passes[i].fog_color >> passes[i].fog_alpha >> passes[i].fog_near >> passes[i].fog_far;
        }
        std::fprintf(stderr, "View Resource \"%s\"\n", name.c_str());
    }
};

class U3D
{
    BitStreamReader reader;
    FileHeader *header;
    PriorityManager priority;
    std::list<ModifierChain *> entities;
    std::list<Resource *> resources;
public:
    U3D(const std::string& filename)
        : reader(filename)
    {
        while(true) {
            uint32_t type = reader.read<uint32_t>();

            switch(type) {
            case 0x00443355:    //File Header Block
                header = new FileHeader(reader);
                break;
            case 0xFFFFFF14:    //Modifier Chain Block
                entities.push_back(new ModifierChain(reader));
                break;
            case 0xFFFFFF15:    //Priority Update Block
                priority.read_update_block(reader);
                break;
            case 0xFFFFFF16:    //New Object Type Block
                std::fprintf(stderr, "New Object Type block is not supported in the current version.\n");
                return;
            case 0xFFFFFF51:    //Light Resource Block
                resources.push_back(new LightResource(reader));
                break;
            case 0xFFFFFF52:    //View Resource Block
                resources.push_back(new ViewResource(reader));
                break;
            case 0xFFFFFF53:    //Lit Texture Shader Block
                resources.push_back(new LitTextureShader(reader));
                break;
            case 0xFFFFFF54:    //Material Block
                resources.push_back(new Material(reader));
                break;
            case 0xFFFFFF55:    //Texture Declaration
                break;
            case 0xFFFFFF5C:    //Texture Continuation
                break;
            case 0xFFFFFF3B:    //CLOD Base Mesh Continuation
                break;
            case 0xFFFFFF3C:    //CLOD Progressive Mesh Continuation
                break;
            default:
                if(0x00000100 <= type && type <= 0x00FFFFFF) {
                    std::fprintf(stderr, "New Object block is not supported in the current version.\n");
                } else {
                    std::fprintf(stderr, "Unknown block type: 0x%08X.\n", type);
                }
                return;
            }
        }
    }
};

int main(int argc, char * const argv[])
{
    std::printf("Universal 3D loader v0.1a\n");

    if(argc < 2) {
        std::fprintf(stderr, "Please specify an input file.\n");
        return 1;
    }

    U3D u3d(argv[1]);

    std::fprintf(stderr, "%s successfully parsed.\n", argv[1]);
    
    return 0;
}
