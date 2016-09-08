#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <vector>
#include <unordered_map>

#include "types.hpp"
#include "bitstream.hpp"

namespace U3D
{

class Modifier
{
public:
    Modifier() {}
    virtual ~Modifier() {}
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
    std::vector<std::vector<std::string>> shader_names;
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

class CLOD_Mesh : public Modifier
{
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
        reader >> chain_index;
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
    }
    uint32_t get_texlayer_count(uint32_t shading_id) const
    {
        return maxmesh.shading_descs[shading_id].texcoord_dims.size();
    }
    bool is_diffuse_color_present(uint32_t shading_id) const
    {
        return maxmesh.shading_descs[shading_id].attributes & 0x00000001;
    }
    bool is_specular_color_present(uint32_t shading_id) const
    {
        return maxmesh.shading_descs[shading_id].attributes & 0x00000002;
    }
    bool is_normals_excluded() const
    {
        return maxmesh.attributes & 0x00000001;
    }
};

class CLOD_Modifier : public Modifier
{
    uint32_t chain_index, attributes;
    float LOD_bias, level;
public:
    CLOD_Modifier(BitStreamReader& reader)
    {
        reader >> chain_index >> attributes >> LOD_bias >> level;
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
        for(auto p : *this) delete p;
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

class CLOD_BaseMesh
{
    std::vector<Vector3f> positions, normals;
    std::vector<Color4f> diffuse_colors, specular_colors;
    std::vector<TexCoord4f> texcoords;
    struct Corner
    {
        uint32_t position, normal;
        uint32_t diffuse, specular, texcoord[8];
    };
    struct Face
    {
        uint32_t shading_id;
        Corner corners[3];
    };
    std::vector<Face> faces;
public:
    CLOD_BaseMesh(BitStreamReader& reader, const CLOD_Mesh& mesh)
    {
        uint32_t face_count, position_count, normal_count;
        uint32_t diffuse_count, specular_count, texcoord_count;
        reader.read<uint32_t>();    //Chain index is always zero.
        reader >> face_count >> position_count >> normal_count >> diffuse_count >> specular_count >> texcoord_count;
        positions.resize(position_count);
        for(unsigned int i = 0; i < position_count; i++) reader >> positions[i];
        normals.resize(normal_count);
        for(unsigned int i = 0; i < normal_count; i++) reader >> normals[i];
        diffuse_colors.resize(diffuse_count);
        for(unsigned int i = 0; i < diffuse_count; i++) reader >> diffuse_colors[i];
        specular_colors.resize(specular_count);
        for(unsigned int i = 0; i < specular_count; i++) reader >> specular_colors[i];
        texcoords.resize(texcoord_count);
        for(unsigned int i = 0; i < texcoord_count; i++) reader >> texcoords[i];
        faces.resize(face_count);
        for(unsigned int i = 0; i < face_count; i++) {
            reader[ContextEnum::cShading] >> faces[i].shading_id;
            for(int j = 0; j < 3; j++) {
                reader[position_count] >> faces[i].corners[j].position;
                reader[normal_count] >> faces[i].corners[j].normal;
                reader[diffuse_count] >> faces[i].corners[j].diffuse;
                reader[specular_count] >> faces[i].corners[j].specular;
                for(int k = 0; k < mesh.get_texlayer_count(faces[i].shading_id); k++) {
                    reader[texcoord_count] >> faces[i].corners[j].texcoord[k];
                }
            }
        }
    }
};

class CLOD_ProgressiveMesh
{
    uint32_t start, end;
public:
    CLOD_ProgressiveMesh(BitStreamReader& reader, const CLOD_Mesh& mesh)
    {
        reader.read<uint32_t>();    //Chain index is always zero.
        reader >> start >> end;
        for(unsigned int i = start; i < end; i++) {
            std::fprintf(stderr, "Resolution update %u\n", i);
            uint32_t split_position;
            if(i == 0) {
                reader[ContextEnum::cZero] >> split_position;
            } else {
                reader[i] >> split_position;
            }
            std::fprintf(stderr, "Split Position = %u.\n", split_position);
            uint16_t new_diffuse_count = reader[ContextEnum::cDiffuseCount].read<uint16_t>();
            std::fprintf(stderr, "Diffuse Count = %u.\n", new_diffuse_count);
            for(unsigned int j = 0; j < new_diffuse_count; j++) {
                uint8_t diffuse_sign = reader[ContextEnum::cDiffuseColorSign].read<uint8_t>();
                uint32_t diffuse_red = reader[ContextEnum::cColorDiffR].read<uint32_t>();
                uint32_t diffuse_green = reader[ContextEnum::cColorDiffG].read<uint32_t>();
                uint32_t diffuse_blue = reader[ContextEnum::cColorDiffB].read<uint32_t>();
                uint32_t diffuse_alpha = reader[ContextEnum::cColorDiffA].read<uint32_t>();

                std::fprintf(stderr, "%u %u %u %u %u\n", diffuse_sign, diffuse_red, diffuse_green, diffuse_blue, diffuse_alpha);
            }
            uint16_t new_specular_count = reader[ContextEnum::cSpecularCount].read<uint16_t>();
            std::fprintf(stderr, "Specular Count = %u.\n", new_specular_count);
            for(unsigned int j = 0; j < new_diffuse_count; j++) {
                uint8_t specular_sign = reader[ContextEnum::cSpecularColorSign].read<uint8_t>();
                uint32_t specular_red = reader[ContextEnum::cColorDiffR].read<uint32_t>();
                uint32_t specular_green = reader[ContextEnum::cColorDiffG].read<uint32_t>();
                uint32_t specular_blue = reader[ContextEnum::cColorDiffB].read<uint32_t>();
                uint32_t specular_alpha = reader[ContextEnum::cColorDiffA].read<uint32_t>();

                std::fprintf(stderr, "%u %u %u %u %u\n", specular_sign, specular_red, specular_green, specular_blue, specular_alpha);
            }
            uint16_t new_texcoord_count = reader[ContextEnum::cTexCoordCount].read<uint16_t>();
            std::fprintf(stderr, "TexCoord Count = %u.\n", new_texcoord_count);
            for(unsigned int j = 0; j < new_texcoord_count; j++) {
                uint8_t texcoord_sign = reader[ContextEnum::cTexCoordSign].read<uint8_t>();
                uint32_t texcoord_U = reader[ContextEnum::cTexCDiffU].read<uint32_t>();
                uint32_t texcoord_V = reader[ContextEnum::cTexCDiffV].read<uint32_t>();
                uint32_t texcoord_S = reader[ContextEnum::cTexCDiffS].read<uint32_t>();
                uint32_t texcoord_T = reader[ContextEnum::cTexCDiffT].read<uint32_t>();

                std::fprintf(stderr, "%u %u %u %u %u\n", texcoord_sign, texcoord_U, texcoord_V, texcoord_S, texcoord_T);
            }
            uint32_t new_face_count = reader[ContextEnum::cFaceCnt].read<uint32_t>();
            std::fprintf(stderr, "Face Count = %u.\n", new_face_count);
            for(unsigned int j = 0; j < new_face_count; j++) {
                uint32_t shading_id = reader[ContextEnum::cShading].read<uint32_t>();
                uint8_t face_ornt = reader[ContextEnum::cFaceOrnt].read<uint8_t>();
                uint8_t third_pos_type = reader[ContextEnum::cThrdPosType].read<uint8_t>();
                uint32_t third_pos;
                if(third_pos_type == 1) {
                    third_pos = reader[ContextEnum::cLocal3rdPos].read<uint32_t>();
                } else {
                    third_pos = reader[i].read<uint32_t>();
                }
                std::fprintf(stderr, "Shading ID = %u, Orientation = %u, 3rd pos = %u.\n", shading_id, face_ornt, third_pos);
            }
            for(unsigned int j = 0; j < new_face_count; j++) {
                if(mesh.is_diffuse_color_present(shading_id)) {
                }
                if(mesh.is_specular_color_present(shading_id)) {
                }
                if(mesh.get_texlayer_count(shading_id) > 0) {
                }
            }
            uint8_t pos_sign = reader[ContextEnum::cPosDiffSign].read<uint8_t>();
            uint32_t pos_X = reader[ContextEnum::cPosDiffX].read<uint32_t>();
            uint32_t pos_Y = reader[ContextEnum::cPosDiffY].read<uint32_t>();
            uint32_t pos_Z = reader[ContextEnum::cPosDiffZ].read<uint32_t>();
            std::fprintf(stderr, "New Position = %u %u %u %u\n", pos_sign, pos_X, pos_Y, pos_Z);
            /*if(!mesh.is_normals_excluded()) {
                uint32_t new_normal_count = reader[ContextEnum::cNormalCnt].read<uint32_t>();
                std::fprintf(stderr, "Normal Count = %u.\n", new_normal_count);
                for(unsigned int j = 0; j < new_normal_count; j++) {
                    uint8_t normal_sign = reader[ContextEnum::cDiffNormalSign].read<uint8_t>();
                    uint32_t normal_X = reader[ContextEnum::cDiffNormalX].read<uint32_t>();
                    uint32_t normal_Y = reader[ContextEnum::cDiffNormalY].read<uint32_t>();
                    uint32_t normal_Z = reader[ContextEnum::cDiffNormalZ].read<uint32_t>();
                }
            }
            return;*/
        }
    }
};

class U3DContext
{
    FileHeader header;
    PriorityManager priority;
    std::unordered_map<std::string, ModelResource *> models;
    std::unordered_map<std::string, LightResource *> lights;
    std::unordered_map<std::string, ViewResource *> views;
    std::unordered_map<std::string, Texture *> textures;
    std::unordered_map<std::string, LitTextureShader *> shaders;
    std::unordered_map<std::string, Material *> materials;
    std::unordered_map<std::string, Node *> nodes;
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
            /*case 0xFFFFFF5C:    //Texture Continuation
                break;*/
            case 0xFFFFFF3B:    //CLOD Base Mesh Continuation
                name = reader.read_str();
                if(models[name] != nullptr) {
                    CLOD_Mesh *decl = dynamic_cast<CLOD_Mesh *>(models[name]->front());
                    if(decl != nullptr) {
                        //decl->register_base(new CLOD_BaseMesh(reader, decl));
                        new CLOD_BaseMesh(reader, *decl);
                        std::fprintf(stderr, "CLOD Base Mesh Continuation \"%s\"\n", name.c_str());
                    }
                }
                break;
            case 0xFFFFFF3C:    //CLOD Progressive Mesh Continuation
                name = reader.read_str();
                if(models[name] != nullptr) {
                    CLOD_Mesh *decl = dynamic_cast<CLOD_Mesh *>(models[name]->front());
                    if(decl != nullptr) {
                        //decl->register_update(new CLOD_ProgressiveMesh(reader, decl));
                        new CLOD_ProgressiveMesh(reader, *decl);
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
        for(auto p : models) delete p.second;
        for(auto p : lights) delete p.second;
        for(auto p : views) delete p.second;
        for(auto p : textures) delete p.second;
        for(auto p : shaders) delete p.second;
        for(auto p : materials) delete p.second;
        for(auto p : nodes) delete p.second;
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
