#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <cmath>
#include <vector>
#include <unordered_map>

template<typename T> struct Vector3
{
    T x, y, z;
};

template<typename T> struct Vector2
{
    T u, v;
};

template<typename T> struct Color3
{
    T r, g, b;
};

template<typename T> struct Matrix4
{
    T m[4][4];
};

template<typename T> struct Quaternion4
{
    T w, x, y, z;
};

using Vector3f = Vector3<float>;
using Matrix4f = Matrix4<float>;
using Color3f = Color3<float>;
using Quaternion4f = Quaternion4<float>;
using Vector2f = Vector2<float>;

class BinaryReader
{
    std::ifstream ifs;
    size_t byte_position;
public:
    BinaryReader(const std::string& filename)
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
    std::string read_str()
    {
        std::string ret;

        uint16_t size = read<uint16_t>();

        for(int i = 0; i < size; i++) {
            ret.push_back(ifs.get());
        }
        byte_position += size;

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

class Block
{
    BinaryReader& reader;
    uint32_t data_size, metadata_size;
    size_t position;
private:
    template<typename T, typename U> static T alignto(T a, U n) { return ((a + n - 1) / n) * n; }
public:
    Block(BinaryReader& reader) : reader(reader) {
        data_size = reader.read<uint32_t>();
        metadata_size = reader.read<uint32_t>();
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
    FileHeader(BinaryReader& reader) {
        Block block(reader);
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
        }
        std::fprintf(stderr, "FileHeaderBlock created.\n");
    }
};

class PriorityManager
{
    uint32_t priority;
public:
    PriorityManager() : priority(0) {}
    uint32_t read_update_block(BinaryReader& reader) {
        Block block(reader);
        priority = reader.read<uint32_t>();
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
    void read(BinaryReader& reader)
    {
        name = reader.read_str();
        uint32_t parent_count = reader.read<uint32_t>();
        parents.resize(parent_count);
        for(unsigned int i = 0; i < parent_count; i++) {
            parents[i].name = reader.read_str();
            parents[i].transform = reader.read_matrix4f();
        }
    }
};

class Group : public Node, public Modifier
{
public:
    Group(BinaryReader& reader)
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
    View(BinaryReader& reader)
    {
        Block block(reader);
        Node::read(reader);
        resource_name = reader.read_str();
        attributes = reader.read<uint32_t>();
        near_clipping = reader.read<float>();
        far_clipping = reader.read<float>();
        switch(attributes & 0x6) {
        case 0: //Three-point perspective projection
            projection = reader.read<float>();
            break;
        case 2: //Orthographic projection
            ortho_height = reader.read<float>();
            break;
        case 4: //One-point perspective projection
        case 6: //Two-point perspective projection
            proj_vector = reader.read_vector3f();
            break;
        }
        port_w = reader.read<float>();
        port_h = reader.read<float>();
        port_x = reader.read<float>();
        port_y = reader.read<float>();
        uint32_t backdrop_count = reader.read<uint32_t>();
        backdrops.resize(backdrop_count);
        for(unsigned int i = 0; i < backdrop_count; i++) {
            backdrops[i].texture_name = reader.read_str();
            backdrops[i].blend = reader.read<float>();
            backdrops[i].location_x = reader.read<float>();
            backdrops[i].location_y = reader.read<float>();
            backdrops[i].reg_x = reader.read<int32_t>();
            backdrops[i].reg_y = reader.read<int32_t>();
            backdrops[i].scale_x = reader.read<float>();
            backdrops[i].scale_y = reader.read<float>();
        }
        uint32_t overlay_count = reader.read<uint32_t>();
        overlays.resize(overlay_count);
        for(unsigned int i = 0; i < overlay_count; i++) {
            overlays[i].texture_name = reader.read_str();
            overlays[i].blend = reader.read<float>();
            overlays[i].location_x = reader.read<float>();
            overlays[i].location_y = reader.read<float>();
            overlays[i].reg_x = reader.read<int32_t>();
            overlays[i].reg_y = reader.read<int32_t>();
            overlays[i].scale_x = reader.read<float>();
            overlays[i].scale_y = reader.read<float>();
        }

        std::fprintf(stderr, "\tView \"%s\" -> \"%s\"\n", name.c_str(), resource_name.c_str());
    }
};

class Model : public Node, public Modifier
{
    std::string resource_name;
    uint32_t visibility;
public:
    Model(BinaryReader& reader)
    {
        Block block(reader);
        Node::read(reader);
        resource_name = reader.read_str();
        visibility = reader.read<uint32_t>();

        std::fprintf(stderr, "\tModel \"%s\" -> \"%s\"\n", name.c_str(), resource_name.c_str());
    }
};

class Light : public Node, public Modifier
{
    std::string resource_name;
public:
    Light(BinaryReader& reader)
    {
        Block block(reader);
        Node::read(reader);
        resource_name = reader.read_str();

        std::fprintf(stderr, "\tLight \"%s\" -> \"%s\"\n", name.c_str(), resource_name.c_str());
    }
};

class Shading : public Modifier
{
    std::string name;
    uint32_t chain_index, attributes;
    std::vector<std::vector<std::string>> shader_names;
public:
    Shading(BinaryReader& reader)
    {
        Block block(reader);
        name = reader.read_str();
        chain_index = reader.read<uint32_t>();
        attributes = reader.read<uint32_t>();
        uint32_t list_count = reader.read<uint32_t>();
        shader_names.resize(list_count);
        for(unsigned int i = 0; i < list_count; i++) {
            uint32_t shader_count = reader.read<uint32_t>();
            shader_names[i].resize(shader_count);
            for(unsigned int j = 0; j < shader_count; j++) {
                shader_names[i][j] = reader.read_str();
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
    CLOD_Mesh(BinaryReader& reader)
    {
        Block block(reader);
        name = reader.read_str();
        chain_index = reader.read<uint32_t>();
        maxmesh.attributes = reader.read<uint32_t>();
        maxmesh.face_count = reader.read<uint32_t>();
        maxmesh.position_count = reader.read<uint32_t>();
        maxmesh.normal_count = reader.read<uint32_t>();
        maxmesh.diffuse_count = reader.read<uint32_t>();
        maxmesh.specular_count = reader.read<uint32_t>();
        maxmesh.texcoord_count = reader.read<uint32_t>();
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
        min_res = reader.read<uint32_t>();
        max_res = reader.read<uint32_t>();
        position_quality = reader.read<uint32_t>();
        normal_quality = reader.read<uint32_t>();
        texcoord_quality = reader.read<uint32_t>();
        position_iq = reader.read<float>();
        normal_iq = reader.read<float>();
        texcoord_iq = reader.read<float>();
        diffuse_iq = reader.read<float>();
        specular_iq = reader.read<float>();
        normal_crease = reader.read<float>();
        normal_update = reader.read<float>();
        normal_tolerance = reader.read<float>();
        uint32_t bone_count = reader.read<uint32_t>();
        skeleton.resize(bone_count);
        for(unsigned int i = 0; i < bone_count; i++) {
            skeleton[i].name = reader.read_str();
            skeleton[i].parent_name = reader.read_str();
            skeleton[i].attributes = reader.read<uint32_t>();
            skeleton[i].length = reader.read<float>();
            skeleton[i].displacement = reader.read_vector3f();
            skeleton[i].orientation = reader.read_quaternion4f();
            if(skeleton[i].attributes & 0x00000001) {
                skeleton[i].link_count = reader.read<uint32_t>();
                skeleton[i].link_length = reader.read<float>();
            }
            if(skeleton[i].attributes & 0x00000002) {
                skeleton[i].start_joint_center = reader.read_vector2f();
                skeleton[i].start_joint_scale = reader.read_vector2f();
                skeleton[i].end_joint_center = reader.read_vector2f();
                skeleton[i].end_joint_scale = reader.read_vector2f();
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
    ModifierChain(BinaryReader& reader) {
        Block block(reader);
        name = reader.read_str();
        type = reader.read<uint32_t>();
        attribute = reader.read<uint32_t>();
        if(attribute & 0x00000001) {
            bsphere = reader.read_vector3f();
            bsphere_radius = reader.read<float>();
        } else if(attribute & 0x00000002) {
            aabb_min = reader.read_vector3f();
            aabb_max = reader.read_vector3f();
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
    LitTextureShader(BinaryReader& reader)
    {
        Block block(reader);
        name = reader.read_str();
        attributes = reader.read<uint32_t>();
        alpha_reference = reader.read<float>();
        alpha_function = reader.read<uint32_t>();
        blend_function = reader.read<uint32_t>();
        render_pass_flags = reader.read<uint32_t>();
        shader_channels = reader.read<uint32_t>();
        alpha_texture_channels = reader.read<uint32_t>();
        material_name = reader.read_str();
        for(unsigned int i = 0; i < 8; i++) {
            if(shader_channels & (1 << i)) {
                texinfos[i].name = reader.read_str();
                texinfos[i].intensity = reader.read<float>();
                texinfos[i].blend_function = reader.read<uint8_t>();
                texinfos[i].blend_source = reader.read<uint8_t>();
                texinfos[i].blend_constant = reader.read<float>();
                texinfos[i].mode = reader.read<uint8_t>();
                texinfos[i].transform = reader.read_matrix4f();
                texinfos[i].wrap_transform = reader.read_matrix4f();
                texinfos[i].repeat = reader.read<uint8_t>();
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
    Material(BinaryReader& reader)
    {
        Block block(reader);
        name = reader.read_str();
        attributes = reader.read<uint32_t>();
        ambient = reader.read_color3f();
        diffuse = reader.read_color3f();
        specular = reader.read_color3f();
        emissive = reader.read_color3f();
        reflectivity = reader.read<float>();
        opacity = reader.read<float>();

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
    LightResource(BinaryReader& reader)
    {
        Block block(reader);
        name = reader.read_str();
        attributes = reader.read<uint32_t>();
        type = reader.read<uint8_t>();
        color = reader.read_color3f();
        reader.read<float>();
        att_constant = reader.read<float>();
        att_linear = reader.read<float>();
        att_quadratic = reader.read<float>();
        spot_angle = reader.read<float>();
        intensity = reader.read<float>();

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
    ViewResource(BinaryReader& reader)
    {
        Block block(reader);
        name = reader.read_str();
        uint32_t pass_count = reader.read<uint32_t>();
        passes.resize(pass_count);
        for(unsigned int i = 0; i < pass_count; i++) {
            passes[i].root_node_name = reader.read_str();
            passes[i].render_attributes = reader.read<uint32_t>();
            passes[i].fog_mode = reader.read<uint32_t>();
            passes[i].fog_color = reader.read_color3f();
            passes[i].fog_alpha = reader.read<float>();
            passes[i].fog_near = reader.read<float>();
            passes[i].fog_far = reader.read<float>();
        }
        std::fprintf(stderr, "View Resource \"%s\"\n", name.c_str());
    }
};

class U3D
{
    BinaryReader reader;
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
