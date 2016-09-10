#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <vector>
#include <map>

#include "types.hpp"
#include "bitstream.hpp"
#include "mesh.hpp"

namespace U3D
{

class FileHeader
{
    uint16_t major_version, minor_version;
    uint32_t profile_identifier;
    uint32_t declaration_size;
    uint64_t file_size;
    uint32_t character_encoding;
    double units_scaling_factor;
public:
    void read_header_block(BitStreamReader& reader) {
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
    std::vector<Parent> parents;
public:
    void read(BitStreamReader& reader)
    {
        uint32_t parent_count = reader.read<uint32_t>();
        parents.resize(parent_count);
        for(unsigned int i = 0; i < parent_count; i++) {
            reader >> parents[i].name >> parents[i].transform;
        }
    }
    virtual ~Node() {}
};

class Group : public Node, public Modifier
{
public:
    Group(BitStreamReader& reader)
    {
        Node::read(reader);
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

    }
};

class Model : public Node, public Modifier
{
    std::string resource_name;
    uint32_t visibility;
public:
    Model(BitStreamReader& reader)
    {
        Node::read(reader);
        reader >> resource_name >> visibility;
    }
};

class Light : public Node, public Modifier
{
    std::string resource_name;
public:
    Light(BitStreamReader& reader)
    {
        Node::read(reader);
        reader >> resource_name;
    }
};

class Shading : public Modifier
{
    uint32_t chain_index, attributes;
    std::vector<std::vector<std::string> > shader_names;
public:
    Shading(BitStreamReader& reader)
    {
        reader >> chain_index >> attributes;
        uint32_t list_count = reader.read<uint32_t>();
        shader_names.resize(list_count);
        for(unsigned int i = 0; i < list_count; i++) {
            uint32_t shader_count = reader.read<uint32_t>();
            shader_names[i].resize(shader_count);
            for(unsigned int j = 0; j < shader_count; j++) {
                reader >> shader_names[i][j];
            }
        }
    }
};

class ModifierChain : public std::vector<Modifier *>
{
    uint32_t attribute;
    Vector3f bsphere;
    float bsphere_radius;
    Vector3f aabb_min, aabb_max;
public:
    ModifierChain(BitStreamReader& reader) {
        reader >> attribute;
        if(attribute & 0x00000001) {
            reader >> bsphere >> bsphere_radius;
        } else if(attribute & 0x00000002) {
            reader >> aabb_min >> aabb_max;
        }
        reader.align_to_word();
        uint32_t modifier_count = reader.read<uint32_t>();
        resize(modifier_count);
        for(unsigned int i = 0; i < modifier_count; i++) {
            BitStreamReader::SubBlock subblock(reader);
            std::string name = reader.read_str();
            switch(subblock.get_type()) {
            case 0xFFFFFF21:    //Group Node Block
                (*this)[i] = new Group(reader);
                std::fprintf(stderr, "\tGroup node \"%s\"\n", name.c_str());
                break;
            case 0xFFFFFF22:    //Model Node Block
                (*this)[i] = new Model(reader);
                std::fprintf(stderr, "\tModel \"%s\"\n", name.c_str());
                break;
            case 0xFFFFFF23:    //Light Node Block
                (*this)[i] = new Light(reader);
                std::fprintf(stderr, "\tLight \"%s\"\n", name.c_str());
                break;
            case 0xFFFFFF24:    //View Node Block
                (*this)[i] = new View(reader);
                std::fprintf(stderr, "\tView \"%s\"\n", name.c_str());
                break;
            case 0xFFFFFF31:    //CLOD Mesh Generator Declaration
                (*this)[i] = new CLOD_Mesh(reader);
                std::fprintf(stderr, "\tCLOD Mesh \"%s\"\n", name.c_str());
                break;
            case 0xFFFFFF36:    //Point Set Declaration
            case 0xFFFFFF37:    //Line Set Declaration
            case 0xFFFFFF41:    //2D Glyph Modifier Block
            case 0xFFFFFF42:    //Subdivision Modifier Block
            case 0xFFFFFF43:    //Animation Modifier Block
            case 0xFFFFFF44:    //Bone Weight Modifier Block
                std::fprintf(stderr, "Modifier Type 0x%08X is not implemented in the current version.\n", subblock.get_type());
                return;
            case 0xFFFFFF45:    //Shading Modifier Block
                (*this)[i] = new Shading(reader);
                std::fprintf(stderr, "\tShading \"%s\"\n", name.c_str());
                break;
            case 0xFFFFFF46:    //CLOD Modifier Block
                (*this)[i] = new CLOD_Modifier(reader);
                std::fprintf(stderr, "\tCLOD Modifier \"%s\"\n", name.c_str());
                break;
            default:
                std::fprintf(stderr, "Illegal modifier type: 0x%08X.\n", subblock.get_type());
                return;
            }
        }
    }
    virtual ~ModifierChain() {
        for(iterator p = begin(); p != end(); p++) delete *p;
    }
};

class LitTextureShader
{
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
        reader >> attributes >> alpha_reference >> alpha_function >> blend_function;
        reader >> render_pass_flags >> shader_channels >> alpha_texture_channels >> material_name;
        for(unsigned int i = 0; i < 8; i++) {
            if(shader_channels & (1 << i)) {
                reader >> texinfos[i].name >> texinfos[i].intensity >> texinfos[i].blend_function >> texinfos[i].blend_source;
                reader >> texinfos[i].blend_constant >> texinfos[i].mode >> texinfos[i].transform >> texinfos[i].wrap_transform;
                reader >> texinfos[i].repeat;
            }
        }
    }
};

class Material
{
    uint32_t attributes;
    Color3f ambient, diffuse, specular, emissive;
    float reflectivity, opacity;
public:
    Material(BitStreamReader& reader)
    {
        reader >> attributes >> ambient >> diffuse >> specular >> emissive >> reflectivity >> opacity;
    }
};

class LightResource
{
    uint32_t attributes;
    uint8_t type;
    Color3f color;
    float att_constant, att_linear, att_quadratic;
    float spot_angle, intensity;
public:
    LightResource(BitStreamReader& reader)
    {
        reader >> attributes >> type >> color;
        reader.read<float>();
        reader >> att_constant >> att_linear >> att_quadratic >> spot_angle >> intensity;
    }
};

class ViewResource
{
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
        uint32_t pass_count = reader.read<uint32_t>();
        passes.resize(pass_count);
        for(unsigned int i = 0; i < pass_count; i++) {
            reader >> passes[i].root_node_name >> passes[i].render_attributes;
            reader >> passes[i].fog_mode >> passes[i].fog_color >> passes[i].fog_alpha >> passes[i].fog_near >> passes[i].fog_far;
        }
    }
};

class Texture
{
    uint32_t width, height;
    uint8_t type;
    struct Continuation
    {
        uint8_t compression_type, channels;
        uint16_t attributes;
        uint32_t byte_count;
    };
    std::vector<Continuation> continuations;
public:
    Texture(BitStreamReader& reader)
    {
        reader >> height >> width >> type;
        uint32_t continuation_count = reader.read<uint32_t>();
        continuations.resize(continuation_count);
        for(unsigned int i = 0; i < continuation_count; i++) {
            reader >> continuations[i].compression_type >> continuations[i].channels >> continuations[i].attributes;
            if(continuations[i].attributes & 0x0001) {
                std::fprintf(stderr, "External texture reference is not implemented in the current version.\n");
                return;
            } else {
                reader >> continuations[i].byte_count;
            }
        }
    }
};

class NodeModifierChain : public Node, public ModifierChain
{
public:
    NodeModifierChain(BitStreamReader& reader) : ModifierChain(reader)
    {
    }
};

class ModelResource : public ModifierChain
{
public:
    ModelResource(BitStreamReader& reader) : ModifierChain(reader)
    {
    }
};

/*class TextureModifierChain : public Texture, public ModifierChain
{
public:
    TextureModifierChain(BitStreamReader& reader) : ModifierChain(reader)
    {
    }
};*/

class U3DContext
{
    FileHeader header;
    PriorityManager priority;
    std::map<std::string, ModelResource *> models;
    std::map<std::string, LightResource *> lights;
    std::map<std::string, ViewResource *> views;
    std::map<std::string, Texture *> textures;
    std::map<std::string, LitTextureShader *> shaders;
    std::map<std::string, Material *> materials;
    std::map<std::string, Node *> nodes;
    BitStreamReader reader;
public:
    U3DContext(const std::string& filename) : reader(filename)
    {
        while(reader.open_block()) {
            std::string name;

            switch(reader.get_type()) {
            case 0x00443355:    //File Header Block
                header.read_header_block(reader);
                break;
            case 0xFFFFFF14:    //Modifier Chain Block
                name = reader.read_str();
                std::fprintf(stderr, "Modifier Chain \"%s\"\n", name.c_str());
                switch(reader.read<uint32_t>()) {
                case 0:
                    nodes[name] = new NodeModifierChain(reader);
                    break;
                case 1:
                    models[name] = new ModelResource(reader);
                    break;
                case 2:
                    //textures[name] = new TextureModifierChain(reader);
                    break;
                }
                break;
            case 0xFFFFFF15:    //Priority Update Block
                priority.read_update_block(reader);
                break;
            case 0xFFFFFF16:    //New Object Type Block
                std::fprintf(stderr, "New Object Type block is not supported in the current version.\n");
                return;
            case 0xFFFFFF51:    //Light Resource Block
                name = reader.read_str();
                lights[name] = new LightResource(reader);
                std::fprintf(stderr, "Light Resource \"%s\"\n", name.c_str());
                break;
            case 0xFFFFFF52:    //View Resource Block
                name = reader.read_str();
                views[name] = new ViewResource(reader);
                std::fprintf(stderr, "View Resource \"%s\"\n", name.c_str());
                break;
            case 0xFFFFFF53:    //Lit Texture Shader Block
                name = reader.read_str();
                shaders[name] = new LitTextureShader(reader);
                std::fprintf(stderr, "Lit Texture Shader Resource \"%s\"\n", name.c_str());
                break;
            case 0xFFFFFF54:    //Material Block
                name = reader.read_str();
                materials[name] = new Material(reader);
                std::fprintf(stderr, "Material \"%s\"\n", name.c_str());
                break;
            case 0xFFFFFF55:    //Texture Declaration
                name = reader.read_str();
                textures[name] = new Texture(reader);
                std::fprintf(stderr, "Texture Resource \"%s\"\n", name.c_str());
                break;
            case 0xFFFFFF56:    //Motion Declaration
                break;
            case 0xFFFFFF5C:    //Texture Continuation
                break;
            case 0xFFFFFF3B:    //CLOD Base Mesh Continuation
                name = reader.read_str();
                if(models[name] != NULL) {
                    CLOD_Mesh *decl = dynamic_cast<CLOD_Mesh *>(models[name]->front());
                    if(decl != NULL) {
                        decl->create_base_mesh(reader);
                        std::fprintf(stderr, "CLOD Base Mesh Continuation \"%s\"\n", name.c_str());
                    }
                }
                break;
            case 0xFFFFFF3C:    //CLOD Progressive Mesh Continuation
                name = reader.read_str();
                if(models[name] != NULL) {
                    CLOD_Mesh *decl = dynamic_cast<CLOD_Mesh *>(models[name]->front());
                    if(decl != NULL) {
                        decl->update_resolution(reader);
                        std::fprintf(stderr, "CLOD Progressive Mesh Continuation \"%s\"\n", name.c_str());
                    }
                }
                break;
            default:
                if(0x00000100 <= reader.get_type() && reader.get_type() <= 0x00FFFFFF) {
                    std::fprintf(stderr, "New Object block [%08X] is not supported in the current version.\n", reader.get_type());
                } else {
                    std::fprintf(stderr, "Unknown block type: 0x%08X.\n", reader.get_type());
                }
                return;
            }
        }
    }
    ~U3DContext() {
        for(std::map<std::string, ModelResource *>::iterator i = models.begin(); i != models.end(); i++) delete i->second;
        for(std::map<std::string, LightResource *>::iterator i = lights.begin(); i != lights.end(); i++) delete i->second;
        for(std::map<std::string, ViewResource *>::iterator i = views.begin(); i != views.end(); i++) delete i->second;
        for(std::map<std::string, Texture *>::iterator i = textures.begin(); i != textures.end(); i++) delete i->second;
        for(std::map<std::string, LitTextureShader *>::iterator i = shaders.begin(); i != shaders.end(); i++) delete i->second;
        for(std::map<std::string, Material *>::iterator i = materials.begin(); i != materials.end(); i++) delete i->second;
        for(std::map<std::string, Node *>::iterator i = nodes.begin(); i != nodes.end(); i++) delete i->second;
    }
};

}

int main(int argc, char * const argv[])
{
    std::printf("Universal 3D loader v0.1a\n");

    if(argc < 2) {
        std::fprintf(stderr, "Please specify an input file.\n");
        return 1;
    }

    U3D::U3DContext context(argv[1]);

    std::fprintf(stderr, "%s successfully parsed.\n", argv[1]);
    
    return 0;
}
