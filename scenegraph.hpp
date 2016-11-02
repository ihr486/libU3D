#pragma once

namespace U3D
{

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
            Matrix4f projection_matrix;
            if(type == PERSPECTIVE) {
                Matrix4f::create_perspective_projection(projection_matrix, fovx, aspect, near, far);
            } else if(ORTHOGONAL) {
                Matrix4f::create_orthogonal_projection(projection_matrix, height, aspect, near, far);
            }
            Matrix4f modelview_matrix = view_matrix * model_matrix;
            Matrix4f PVM_matrix = projection_matrix * modelview_matrix;
            glUniformMatrix4fv(glGetUniformLocation(program, "PVM_matrix"), (GLfloat *)&PVM_matrix);
            glUniformMatrix4fv(glGetUniformLocation(program, "modelview_matrix"), (GLfloat *)&modelview_matrix);
        }
        void setup(const View& view, const Matrix4f& transform)
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
        void setup(const Light& light, const Matrix4f& transform)
        {
            position = Vector4f(0, 0, 0, 1) * transform;
            direction = Vector4f(0, 0, -1, 0) * transform;
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
        Matrix4f model_matrix;
        void setup(const Model& model, const Matrix4f& transform)
        {
            model_matrix = transform;
        }
    };
    ViewParams view;
    std::vector<LightParams> lights;
    std::vector<ModelParams> models;
public:
};

}
