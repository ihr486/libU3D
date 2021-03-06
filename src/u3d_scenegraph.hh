/*
 * Copyright (C) 2016 Hiroka Ihara
 *
 * This file is part of libU3D.
 *
 * libU3D is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libU3D is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libU3D.  If not, see <http://www.gnu.org/licenses/>.
 */

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
        if(pass_count > 1) {
            U3D_WARNING << "Multipass rendering." << std::endl;
        }
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
        float fog_start, fog_end;
        Color3f fog_color;
        void load(GLuint program, const Matrix4f& model_matrix)
        {
            float viewport[4];
            glGetFloatv(GL_VIEWPORT, viewport);
            float aspect = viewport[2] / viewport[3];
            Matrix4f projection_matrix;
            if(type == PERSPECTIVE) {
                Matrix4f::create_perspective_projection(projection_matrix, fovy, aspect, near, far);
            } else if(type == ORTHOGONAL) {
                Matrix4f::create_orthogonal_projection(projection_matrix, height, aspect, near, far);
            }
            Matrix4f modelview_matrix = view_matrix.inverse() * model_matrix;
            Matrix4f PVM_matrix = projection_matrix * modelview_matrix;
            Matrix4f normal_matrix = modelview_matrix.create_normal_matrix();
            /*U3D_LOG << "Near = " << near << ", far = " << far << std::endl;
            U3D_LOG << "projection_matrix = " << projection_matrix << std::endl;
            U3D_LOG << "model_matrix = " << model_matrix << std::endl;
            U3D_LOG << "view_matrix = " << view_matrix << std::endl;
            U3D_LOG << "PVM_matrix = " << PVM_matrix << std::endl;
            U3D_LOG << "normal_matrix = " << normal_matrix << std::endl;*/
            glUniformMatrix4fv(glGetUniformLocation(program, "PVM_matrix"), 1, GL_FALSE, (GLfloat *)&PVM_matrix);
            glUniformMatrix4fv(glGetUniformLocation(program, "modelview_matrix"), 1, GL_FALSE, (GLfloat *)&modelview_matrix);
            glUniformMatrix4fv(glGetUniformLocation(program, "normal_matrix"), 1, GL_FALSE, (GLfloat *)&normal_matrix);
        }
        ViewParams(const View& view, const ViewResource::Pass& view_pass, const Matrix4f& transform)
        {
            view_matrix = transform;
            fovy = view.projection / 180.0f * 3.1415927f;
            height = view.ortho_height;
            near = view.near_clipping;
            far = view.far_clipping > 1E+6f ? 1E+6f : view.far_clipping;
            if(view.attributes & View::PROJECTION_ORTHO) {
                type = ORTHOGONAL;
            } else if(view.attributes & (View::PROJECTION_ONE_POINT | View::PROJECTION_TWO_POINT)) {
                throw U3D_ERROR << "One- and two- point projections are not supported.";
            } else {
                type = PERSPECTIVE;
            }
            if(view_pass.render_attributes & ViewResource::FOG_ENABLED) {
                fog_start = view_pass.fog_near;
                fog_end = view_pass.fog_far;
                fog_color = view_pass.fog_color;
            }
        }
    };
    struct LightParams
    {
        enum {
            LIGHT_AMBIENT = 0, LIGHT_DIRECTIONAL, LIGHT_POINT, LIGHT_SPOT
        };
        uint8_t type;
        Vector3f position, direction;
        Color3f color;
        float att_constant, att_linear, att_quadratic;
        float spot_angle, intensity;
        LightParams(const LightResource& light, const Matrix4f& transform)
        {
            type = light.type;
            position = transform * Vector3f(0, 0, 0);
            direction = (transform.create_normal_matrix() * Vector3f(0, 0, -1)).normalize();
            color = light.color;
            att_constant = light.att_constant;
            att_linear = light.att_linear;
            att_quadratic = light.att_quadratic;
            spot_angle = light.spot_angle;
            intensity = light.intensity;
        }
        void load(GLuint program, const Matrix4f& view_matrix)
        {
            Vector3f viewspace_position = view_matrix * position;
            Vector3f viewspace_direction = (view_matrix.create_normal_matrix() * direction).normalize();
            /*U3D_LOG << "Position = " << viewspace_position << std::endl;
            U3D_LOG << "Direction = " << viewspace_direction << std::endl;
            U3D_LOG << "Color = " << color << std::endl;
            U3D_LOG << "Intensity = " << intensity << std::endl;
            U3D_LOG << "Constant Attenuation = " << att_constant << std::endl;
            U3D_LOG << "Linear Attenuation = " << att_linear << std::endl;
            U3D_LOG << "Quadratic Attenuation = " << att_quadratic << std::endl;*/
            glUniform4f(glGetUniformLocation(program, "light_color"), color.r, color.g, color.b, 1.0f);
            switch(type) {
            case LIGHT_DIRECTIONAL:
                glUniform4f(glGetUniformLocation(program, "light_direction"), viewspace_direction.x, viewspace_direction.y, viewspace_direction.z, 0.0f);
                glUniform1f(glGetUniformLocation(program, "light_intensity"), intensity);
                break;
            case LIGHT_POINT:
                glUniform4f(glGetUniformLocation(program, "light_position"), viewspace_position.x, viewspace_position.y, viewspace_position.z, 1.0f);
                glUniform1f(glGetUniformLocation(program, "light_intensity"), intensity);
                glUniform1f(glGetUniformLocation(program, "light_att0"), att_constant);
                glUniform1f(glGetUniformLocation(program, "light_att1"), att_linear);
                glUniform1f(glGetUniformLocation(program, "light_att2"), att_quadratic);
                break;
            case LIGHT_SPOT:
                glUniform4f(glGetUniformLocation(program, "light_direction"), viewspace_direction.x, viewspace_direction.y, viewspace_direction.z, 0.0f);
                glUniform4f(glGetUniformLocation(program, "light_position"), viewspace_position.x, viewspace_position.y, viewspace_position.z, 1.0f);
                glUniform1f(glGetUniformLocation(program, "light_intensity"), intensity);
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
        std::vector<std::string> shader_names;
        ModelParams(const Model& model, const ModelResource& model_rsc, const Matrix4f& transform)
        {
            model_matrix = transform;
            name = model.resource_name;
            if(model.shading != NULL) {
                shader_names.assign(model.shading->shader_names.begin(), model.shading->shader_names.end());
            } else if(model_rsc.shading != NULL) {
                shader_names.assign(model_rsc.shading->shader_names.begin(), model_rsc.shading->shader_names.end());
            }
        }
    };
    ViewParams view;
    std::vector<LightParams> lights;
    std::vector<ModelParams> models;
public:
    SceneGraph(const View& view_node, const ViewResource::Pass& view_pass, const Matrix4f& transform)
    : view(view_node, view_pass, transform) {
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
        for(std::vector<ModelParams>::iterator j = models.begin(); j != models.end(); j++) {
            RenderGroup *render_group = context->get_render_group(j->name);
            //U3D_LOG << "Rendering model \"" << j->name << "\"" <<  std::endl;
            for(std::vector<LightParams>::iterator i = lights.begin(); i != lights.end(); i++) {
                //U3D_LOG << "Light type = " << (int)i->type << std::endl;
                for(unsigned int k = 0; k < render_group->elements.size(); k++) {
                    ShaderGroup *shader_group;
                    if(k < j->shader_names.size()) {
                        shader_group = context->get_shader_group(j->shader_names[k]);
                        //U3D_LOG << "Element " << k << ":" << j->shader_names[k] << std::endl;
                    } else {
                        shader_group = context->get_shader_group("");
                        //U3D_LOG << "Element " << k << ":" << std::endl;
                    }
                    GLuint program = shader_group->use(i->type);
                    i->load(program, view.view_matrix.inverse());
                    view.load(program, j->model_matrix);
                    for(int l = 0; l < 8; l++) {
                        if(shader_group->shader_channels & (1 << l)) {
                            glActiveTexture(l);
                            glBindTexture(GL_TEXTURE_2D, context->get_texture(shader_group->texture_names[l]));
                        }
                    }
                    render_group->render(k, program);
                }
            }
        }
    }
    Matrix4f& get_view_matrix()
    {
        return view.view_matrix;
    }
    void set_view_matrix(const Matrix4f& matrix)
    {
        view.view_matrix = matrix;
    }
};

}
