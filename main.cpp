#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <vector>
#include <map>

#include <SDL.h>

#include "types.hpp"
#include "bitstream.hpp"
#include "mesh.hpp"
#include "plset.hpp"
#include "texture.hpp"
#include "viewer.hpp"
#include "shader.hpp"
#include "util.hpp"

namespace U3D
{

class Node
{
public:
    struct Parent {
        std::string name;
        Matrix4f transform;
    };
    std::vector<Parent> parents;
public:
    Node(BitStreamReader& reader)
    {
        uint32_t parent_count = reader.read<uint32_t>();
        parents.resize(parent_count);
        for(unsigned int i = 0; i < parent_count; i++) {
            reader >> parents[i].name >> parents[i].transform;
        }
    }
    Node() {}
    virtual ~Node() {}
};

class Group : public Node
{
public:
    Group(BitStreamReader& reader) : Node(reader) {}
    Group() {}
};

class View : public Node
{
    uint32_t attributes;
    static const uint32_t UNIT_SCREEN_POSITION = 1, PROJECTION_ORTHO = 2, PROJECTION_TWO_POINT = 4, PROJECTION_ONE_POINT = 8;
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
    std::string resource_name;
    View(BitStreamReader& reader) : Node(reader)
    {
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

class Model : public Node
{
    uint32_t visibility;
    static const uint32_t FRONT_VISIBLE = 1, BACK_VISIBLE = 2;
public:
    std::string resource_name;
    Model(BitStreamReader& reader) : Node(reader)
    {
        reader >> resource_name >> visibility;
    }
    void add_shading_modifier(Shading *shading)
    {
        delete shading;
    }
};

class Light : public Node
{
public:
    std::string resource_name;
    Light(BitStreamReader& reader) : Node(reader)
    {
        reader >> resource_name;
    }
};

class Material
{
    uint32_t attributes;
    static const uint32_t AMBIENT = 1, DIFFUSE = 2, SPECULAR = 4, EMISSIVE = 8, REFLECTIVITY = 16, OPACITY = 32;
    Color3f ambient, diffuse, specular, emissive;
    float reflectivity, opacity;
public:
    Material(BitStreamReader& reader)
    {
        reader >> attributes >> ambient >> diffuse >> specular >> emissive >> reflectivity >> opacity;
    }
    Material()
    {
        attributes = 0x0000003F;
        ambient = Color3f(0.75f, 0.75f, 0.75f);
        diffuse = Color3f(0, 0, 0);
        specular = Color3f(0, 0, 0);
        emissive = Color3f(0, 0, 0);
        reflectivity = 0;
        opacity = 1.0f;
    }
    void populate_program(GLuint program)
    {
        glUniform3f(glGetUniformLocation(program, "material_diffuse"), diffuse.r, diffuse.g, diffuse.b);
        glUniform3f(glGetUniformLocation(program, "material_specular"), specular.r, specular.g, specular.b);
        glUniform3f(glGetUniformLocation(program, "material_ambient"), ambient.r, ambient.g, ambient.b);
        glUniform3f(glGetUniformLocation(program, "material_emissive"), emissive.r, emissive.g, emissive.b);
        glUniform1f(glGetUniformLocation(program, "material_reflectivity"), reflectivity);
        glUniform1f(glGetUniformLocation(program, "material_opacity"), opacity);
    }
};

class LightResource
{
    uint32_t attributes;
    static const uint32_t LIGHT_ENABLED = 1, LIGHT_SPECULAR = 2, SPOT_DECAY = 4;
    uint8_t type;
    static const uint8_t LIGHT_AMBIENT = 0, LIGHT_DIRECTIONAL = 1, LIGHT_POINT = 2, LIGHT_SPOT = 3;
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
    LightResource()
    {
        attributes = 0x00000001, type = 0x00, color = Color3f(0.75f, 0.75f, 0.75f);
    }
    void populate_program(GLuint program)
    {
        glUniform3f(glGetUniformLocation(program, "light_color"), color.r, color.g, color.b);
        switch(type) {
        case LIGHT_DIRECTIONAL:
            glUniform1f(glGetUniformLocation(program, "light_intensity"), intensity);
            break;
        case LIGHT_POINT:
            glUniform1f(glGetUniformLocation(program, "light_att0"), att_constant);
            glUniform1f(glGetUniformLocation(program, "light_att1"), att_linear);
            glUniform1f(glGetUniformLocation(program, "light_att2"), att_quadratic);
            break;
        case LIGHT_SPOT:
            glUniform1f(glGetUniformLocation(program, "light_att0"), att_constant);
            glUniform1f(glGetUniformLocation(program, "light_att1"), att_linear);
            glUniform1f(glGetUniformLocation(program, "light_att2"), att_quadratic);
            glUniform1f(glGetUniformLocation(program, "light_spot_angle"), spot_angle);
            break;
        }
    }
};

class ViewResource
{
public:
    struct Pass
    {
        std::string root_node_name;
        uint32_t render_attributes;
        uint32_t fog_mode;
        Color3f fog_color;
        float fog_alpha;
        float fog_near, fog_far;
    };
    static const uint32_t FOG_ENABLED = 1;
    static const uint32_t FOG_EXPONENTIAL = 1, FOG_EXPONENTIAL2 = 2;
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
    ViewResource()
    {
        passes.resize(1);
        passes[0].root_node_name = "";
        passes[0].render_attributes = 0x00000000;
    }
};

static uint32_t read_modifier_count(BitStreamReader& reader)
{
    uint32_t attribute;
    reader >> attribute;
    if(attribute & 0x00000001) {
        reader.read<Vector3f>();
        reader.read<float>();
    } else if(attribute & 0x00000002) {
        reader.read<Vector3f>();
        reader.read<Vector3f>();
    }
    reader.align_to_word();
    return reader.read<uint32_t>();
}

static Node *create_node_modifier_chain(BitStreamReader& reader)
{
    Node *head = NULL;
    uint32_t count = read_modifier_count(reader);
    for(unsigned int i = 0; i < count; i++) {
        BitStreamReader::SubBlock subblock(reader);
        std::string name = reader.read_str();
        switch(subblock.get_type()) {
        case 0xFFFFFF21:    //Group Node Block
            head = static_cast<Node *>(new Group(reader));
            std::fprintf(stderr, "\tGroup node \"%s\"\n", name.c_str());
            break;
        case 0xFFFFFF22:    //Model Node Block
            head = static_cast<Node *>(new Model(reader));
            std::fprintf(stderr, "\tModel \"%s\"\n", name.c_str());
            break;
        case 0xFFFFFF23:    //Light Node Block
            head = static_cast<Node *>(new Light(reader));
            std::fprintf(stderr, "\tLight \"%s\"\n", name.c_str());
            break;
        case 0xFFFFFF24:    //View Node Block
            head = static_cast<Node *>(new View(reader));
            std::fprintf(stderr, "\tView \"%s\"\n", name.c_str());
            break;
        case 0xFFFFFF45:    //Shading Modifier Block
            if(head != NULL) {
                Model *model = dynamic_cast<Model *>(head);
                if(model != NULL) {
                    model->add_shading_modifier(new Shading(reader));
                }
            }
            std::fprintf(stderr, "\tShading \"%s\"\n", name.c_str());
            break;
        default:
            std::fprintf(stderr, "Illegal modifier 0x%08X in a node modifier chain\n", subblock.get_type());
            return head;
        }
    }
    return head;
}

static ModelResource *create_model_modifier_chain(BitStreamReader& reader)
{
    ModelResource *head = NULL;
    uint32_t count = read_modifier_count(reader);
    for(unsigned int i = 0; i < count; i++) {
        BitStreamReader::SubBlock subblock(reader);
        std::string name = reader.read_str();
        switch(subblock.get_type()) {
        case 0xFFFFFF31:    //CLOD Mesh Generator Declaration
            head = static_cast<ModelResource *>(new CLOD_Mesh(reader));
            std::fprintf(stderr, "\tCLOD Mesh \"%s\"\n", name.c_str());
            break;
        case 0xFFFFFF36:    //Point Set Declaration
            head = static_cast<ModelResource *>(new PointSet(reader));
            std::fprintf(stderr, "\tPoint Set \"%s\"\n", name.c_str());
            break;
        case 0xFFFFFF37:    //Line Set Declaration
            head = static_cast<ModelResource *>(new LineSet(reader));
            std::fprintf(stderr, "\tLine Set \"%s\"\n", name.c_str());
            break;
        case 0xFFFFFF42:    //Subdivision Modifier Block
        case 0xFFFFFF43:    //Animation Modifier Block
        case 0xFFFFFF44:    //Bone Weight Modifier Block
        case 0xFFFFFF46:    //CLOD Modifier Block
            std::fprintf(stderr, "Modifier Type 0x%08X is not implemented in the current version.\n", subblock.get_type());
            break;
        default:
            std::fprintf(stderr, "Illegal modifier 0x%08X in an instance modifier chain\n", subblock.get_type());
            return head;
        }
    }
    return head;
}

static Texture *create_texture_modifier_chain(BitStreamReader& reader)
{
    Texture *head = NULL;
    uint32_t count = read_modifier_count(reader);
    for(unsigned int i = 0; i < count; i++) {
        BitStreamReader::SubBlock subblock(reader);
        std::string name = reader.read_str();
        switch(subblock.get_type()) {
        case 0xFFFFFF55:    //Texture Declaration
            head = new Texture(reader);
            std::fprintf(stderr, "Texture Resource \"%s\"\n", name.c_str());
            break;
        default:
            std::fprintf(stderr, "Illegal modifier 0x%08X in an instance modifier chain\n", subblock.get_type());
            return head;
        }
    }
    return head;
}

class ViewAssembly
{
public:
    ViewAssembly() {}
    ~ViewAssembly() {}
};

class FileStructure
{
    std::map<std::string, ModelResource *> models;
    std::map<std::string, LightResource *> lights;
    std::map<std::string, ViewResource *> views;
    std::map<std::string, Texture *> textures;
    std::map<std::string, LitTextureShader *> shaders;
    std::map<std::string, Material *> materials;
    std::map<std::string, Node *> nodes;
    BitStreamReader reader;
private:
    void read_header_block(BitStreamReader& reader) {
        uint16_t major_version, minor_version;
        uint32_t profile_identifier;
        uint32_t declaration_size;
        uint64_t file_size;
        uint32_t character_encoding;
        double units_scaling_factor;
        reader >> major_version >> minor_version >> profile_identifier >> declaration_size >> file_size >> character_encoding;
        units_scaling_factor = (profile_identifier & 0x8) ? reader.read<double>() : 1;
        U3D_LOG << "Scaling factor = " << units_scaling_factor << std::endl;
    }
public:
    FileStructure(const std::string& filename) : reader(filename)
    {
        models[""] = new CLOD_Mesh();
        lights[""] = new LightResource();
        views[""] = new ViewResource();
        textures[""] = new Texture();
        shaders[""] = new LitTextureShader();
        materials[""] = new Material();
        nodes[""] = static_cast<Node *>(new Group());

        while(reader.open_block()) {
            std::string name;

            switch(reader.get_type()) {
            case 0x00443355:    //File Header Block
                read_header_block(reader);
                break;
            case 0xFFFFFF14:    //Modifier Chain Block
                name = reader.read_str();
                std::fprintf(stderr, "Modifier Chain \"%s\"\n", name.c_str());
                switch(reader.read<uint32_t>()) {
                case 0:
                    nodes[name] = create_node_modifier_chain(reader);
                    break;
                case 1:
                    models[name] = create_model_modifier_chain(reader);
                    break;
                case 2:
                    textures[name] = create_texture_modifier_chain(reader);
                    break;
                }
                break;
            case 0xFFFFFF15:    //Priority Update Block
                reader.read<uint32_t>();
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
                name = reader.read_str();
                if(textures[name] != NULL) {
                    Texture *decl = dynamic_cast<Texture *>(textures[name]);
                    if(decl != NULL) {
                        decl->load_continuation(reader);
                        std::fprintf(stderr, "Texture Continuation \"%s\"\n", name.c_str());
                    }
                }
                break;
            case 0xFFFFFF3B:    //CLOD Base Mesh Continuation
                name = reader.read_str();
                if(models[name] != NULL) {
                    CLOD_Mesh *decl = dynamic_cast<CLOD_Mesh *>(models[name]);
                    if(decl != NULL) {
                        decl->create_base_mesh(reader);
                        std::fprintf(stderr, "CLOD Base Mesh Continuation \"%s\"\n", name.c_str());
                    }
                } else {
                    std::fprintf(stderr, "CLOD Base Mesh Continuation \"%s\" is not declared.\n", name.c_str());
                }
                break;
            case 0xFFFFFF3C:    //CLOD Progressive Mesh Continuation
                name = reader.read_str();
                if(models[name] != NULL) {
                    CLOD_Mesh *decl = dynamic_cast<CLOD_Mesh *>(models[name]);
                    if(decl != NULL) {
                        decl->update_resolution(reader);
                        std::fprintf(stderr, "CLOD Progressive Mesh Continuation \"%s\"\n", name.c_str());
                    }
                }
                break;
            case 0xFFFFFF3E:    //Point Set Continuation
                name = reader.read_str();
                if(models[name] != NULL) {
                    PointSet *decl = dynamic_cast<PointSet *>(models[name]);
                    if(decl != NULL) {
                        decl->update_resolution(reader);
                        std::fprintf(stderr, "Point Set Continuation \"%s\"\n", name.c_str());
                    }
                }
                break;
            case 0xFFFFFF3F:    //Line Set Continuation
                name = reader.read_str();
                if(models[name] != NULL) {
                    LineSet *decl = dynamic_cast<LineSet *>(models[name]);
                    if(decl != NULL) {
                        decl->update_resolution(reader);
                        std::fprintf(stderr, "Line Set Continuation \"%s\"\n", name.c_str());
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
    ~FileStructure() {
        for(std::map<std::string, ModelResource *>::iterator i = models.begin(); i != models.end(); i++) delete i->second;
        for(std::map<std::string, LightResource *>::iterator i = lights.begin(); i != lights.end(); i++) delete i->second;
        for(std::map<std::string, ViewResource *>::iterator i = views.begin(); i != views.end(); i++) delete i->second;
        for(std::map<std::string, Texture *>::iterator i = textures.begin(); i != textures.end(); i++) delete i->second;
        for(std::map<std::string, LitTextureShader *>::iterator i = shaders.begin(); i != shaders.end(); i++) delete i->second;
        for(std::map<std::string, Material *>::iterator i = materials.begin(); i != materials.end(); i++) delete i->second;
        for(std::map<std::string, Node *>::iterator i = nodes.begin(); i != nodes.end(); i++) delete i->second;
    }
    Node *get_node(const std::string& name) {
        return nodes[name];
    }
    View *get_view(const std::string& name) {
        return dynamic_cast<View *>(nodes[name]);
    }
    View *get_first_view() {
        for(std::map<std::string, Node *>::iterator i = nodes.begin(); i != nodes.end(); i++) {
            View *view = dynamic_cast<View *>(i->second);
            if(view != NULL) return view;
        }
        return NULL;
    }
    bool get_world_transform(Matrix4f *mat, const Node *node, const Node *root) {
        for(std::vector<Node::Parent>::const_iterator i = node->parents.begin(); i != node->parents.end(); i++) {
            Node *parent = nodes[i->name];
            if(parent == root) {
                *mat *= i->transform;
                return true;
            } else {
                if(get_world_transform(mat, parent, root)) {
                    *mat *= i->transform;
                    return true;
                }
            }
        }
        return false;
    }
    ViewAssembly *create_assembly(const View *view) {
        ViewResource *rsc = views[view->resource_name];
        Node *root_node = nodes[rsc->passes[0].root_node_name];
        for(std::map<std::string, Node *>::iterator i = nodes.begin(); i != nodes.end(); i++) {
            Matrix4f world_transform;
            if(get_world_transform(&world_transform, i->second, root_node)) {
                Light *light = dynamic_cast<Light *>(i->second);
                if(light != NULL) {
                    U3D_LOG << "Light node " << light->resource_name << " found." << std::endl;
                    continue;
                }
                Model *model = dynamic_cast<Model *>(i->second);
                if(model != NULL) {
                    U3D_LOG << "Model node " << model->resource_name << " found." << std::endl;
                    continue;
                }
            }
        }
        ViewAssembly *assembly = new ViewAssembly();
        return assembly;
    }
};

}

#ifdef __WIN32__
#include <windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR lpC, int nC)
#else
int main(int argc, char *argv[])
#endif
{
    std::cout << "Universal 3D loader v0.1a" << std::endl;

#ifndef __WIN32__
    if(argc < 2) {
        std::cerr << "Please specify an input file." << std::endl;
        return 1;
    }

    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Failed to initialize SDL2." << std::endl;
        return 1;
    }

    U3D::FileStructure model(argv[1]);

    std::cerr << argv[1] << " successfully parsed." << std::endl;
#else
    U3D::FileStructure model(lpC);

    std::cerr << (char *)lpC << " successfully parsed." << std::endl;
#endif

    try {
        U3D::View *defaultview = model.get_view("DefaultView");
        if(defaultview != NULL) {
            U3D_LOG << "DefaultView found." << std::endl;
        } else {
            defaultview = model.get_first_view();
            if(defaultview == NULL) {
                throw U3D_ERROR << "No View node was found.";
            }
        }
        U3D::Matrix4f view_matrix;
        if(model.get_world_transform(&view_matrix, defaultview, model.get_node(""))) {
            U3D_LOG << "View matrix = " << std::endl << view_matrix << std::endl;
        }
        if(!view_matrix.is_view()) {
            U3D_WARNING << "View matrix does not seem to be legal." << std::endl;
        }
        U3D::Matrix4f inverse_view = view_matrix.inverse_as_view();
        U3D_LOG << "Inverse view matrix = " << std::endl << inverse_view << std::endl;

        U3D::ViewAssembly *assembly = model.create_assembly(defaultview);
        delete assembly;

        U3D::AutoHandle<SDL_Window *> window(SDL_CreateWindow("Universal 3D testbed", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 480, 360, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE), SDL_DestroyWindow);
        if(!window) {
            throw U3D_ERROR << "Failed to create an SDL2 window.";
        }

        U3D::AutoHandle<SDL_GLContext> context(SDL_GL_CreateContext(window), SDL_GL_DeleteContext);
        if(context == NULL) {
            throw U3D_ERROR << "Failed to create an OpenGL context.";
        }

        {
            Viewer viewer;

            SDL_Event event;
            do {
                while(SDL_PollEvent(&event)) {
                    if(event.type == SDL_QUIT) {
                        break;
                    }
                    if(event.type == SDL_WINDOWEVENT) {
                        if(event.window.event == SDL_WINDOWEVENT_RESIZED) {
                            viewer.resize(event.window.data1, event.window.data2);
                        }
                    }
                }
                viewer.render();

                SDL_GL_SwapWindow(window);
            } while(event.type != SDL_QUIT);
        }
    } catch(const U3D::Error& err) {
        std::cerr << err.what() << std::endl;
    }
    SDL_Quit();
    
    return 0;
}
