#pragma once

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

class SceneGraph;

class View : public Node
{
    friend class SceneGraph;

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
    friend class SceneGraph;

    uint32_t visibility;
    static const uint32_t FRONT_VISIBLE = 1, BACK_VISIBLE = 2;
    Shading *shading;
public:
    std::string resource_name;
    Model(BitStreamReader& reader) : Node(reader), shading(NULL)
    {
        reader >> resource_name >> visibility;
    }
    virtual ~Model()
    {
        if(shading != NULL) delete shading;
    }
    void add_shading_modifier(Shading *shading)
    {
        this->shading = shading;
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

class LightResource
{
    friend class SceneGraph;

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
};

class ViewResource
{
    friend class SceneGraph;
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

class SceneGraph
{
    struct ViewParams
    {
        Matrix4f view_matrix;
        enum {
            PERSPECTIVE = 0, ORTHOGONAL
        } type;
        float fovy, height, near, far;
        void load(GLuint program, const Matrix4f& model_matrix)
        {
            float viewport[4];
            glGetFloatv(GL_VIEWPORT, viewport);
            float aspect = viewport[2] / viewport[3];
            Matrix4f projection_matrix;
            if(type == PERSPECTIVE) {
                Matrix4f::create_perspective_projection(projection_matrix, fovy, aspect, near, far);
            } else if(ORTHOGONAL) {
                Matrix4f::create_orthogonal_projection(projection_matrix, height, aspect, near, far);
            }
            Matrix4f modelview_matrix = view_matrix * model_matrix;
            Matrix4f PVM_matrix = projection_matrix * modelview_matrix;
            glUniformMatrix4fv(glGetUniformLocation(program, "PVM_matrix"), 1, GL_FALSE, (GLfloat *)&PVM_matrix);
            glUniformMatrix4fv(glGetUniformLocation(program, "modelview_matrix"), 1, GL_FALSE, (GLfloat *)&modelview_matrix);
        }
        ViewParams(const View& view, const ViewResource& view_rsc, const Matrix4f& transform)
        {
            this->view_matrix = transform.inverse_as_view();
            fovy = view.projection;
            height = view.ortho_height;
            near = view.near_clipping;
            far = view.far_clipping;
        }
    };
    struct LightParams
    {
        enum {
            LIGHT_AMBIENT = 0, LIGHT_DIRECTIONAL, LIGHT_POINT, LIGHT_SPOT
        } type;
        Vector3f position, direction;
        Color3f color;
        float att_constant, att_linear, att_quadratic;
        float spot_angle, intensity;
        LightParams(const LightResource& light, const Matrix4f& transform)
        {
            position = transform * Vector3f(0, 0, 0);
            direction = transform.create_normal_matrix() * Vector3f(0, 0, -1);
            color = light.color;
            att_constant = light.att_constant;
            att_linear = light.att_linear;
            att_quadratic = light.att_quadratic;
            spot_angle = light.spot_angle;
            intensity = light.intensity;
        }
        void load(GLuint program)
        {
            glUniform3f(glGetUniformLocation(program, "light_color"), color.r, color.g, color.b);
            switch(type) {
            case LIGHT_DIRECTIONAL:
                glUniform3f(glGetUniformLocation(program, "light_direction"), direction.x, direction.y, direction.z);
                glUniform1f(glGetUniformLocation(program, "light_intensity"), intensity);
                break;
            case LIGHT_POINT:
                glUniform3f(glGetUniformLocation(program, "light_position"), position.x, position.y, position.z);
                glUniform1f(glGetUniformLocation(program, "light_att0"), att_constant);
                glUniform1f(glGetUniformLocation(program, "light_att1"), att_linear);
                glUniform1f(glGetUniformLocation(program, "light_att2"), att_quadratic);
                break;
            case LIGHT_SPOT:
                glUniform3f(glGetUniformLocation(program, "light_direction"), direction.x, direction.y, direction.z);
                glUniform3f(glGetUniformLocation(program, "light_position"), position.x, position.y, position.z);
                glUniform1f(glGetUniformLocation(program, "light_att0"), att_constant);
                glUniform1f(glGetUniformLocation(program, "light_att1"), att_linear);
                glUniform1f(glGetUniformLocation(program, "light_att2"), att_quadratic);
                glUniform1f(glGetUniformLocation(program, "light_spot_angle"), spot_angle);
                break;
            }
        }
    };
    struct ModelParams
    {
        std::string name;
        Matrix4f model_matrix;
        ModelParams(const Model& model, const ModelResource& model_rsc, const Matrix4f& transform)
        {
            model_matrix = transform;
            name = model.resource_name;
            if(model.shading != NULL) {
            } else if(model_rsc.shading != NULL) {
            } else {
                U3D_WARNING << "No shading specified for model: " << model.resource_name << std::endl;
            }
        }
    };
    ViewParams view;
    std::vector<LightParams> lights;
    std::vector<ModelParams> models;
public:
    SceneGraph(const View& view_node, const ViewResource& view_rsc, const Matrix4f& transform)
    : view(view_node, view_rsc, transform) {
    }
    void register_light(const LightResource& light, const Matrix4f& transform)
    {
        this->lights.push_back(LightParams(light, transform));
    }
    void register_model(const Model& model, const ModelResource& model_rsc, const Matrix4f& transform)
    {
        this->models.push_back(ModelParams(model, model_rsc, transform));
    }
    void render(GraphicsContext *context)
    {
        for(std::vector<LightParams>::iterator i = lights.begin(); i != lights.end(); i++) {
            for(std::vector<ModelParams>::iterator j = models.begin(); j != models.end(); j++) {
            }
        }
    }
};

}
