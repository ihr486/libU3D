#pragma once

namespace U3D
{

class SceneGraph
{
    struct ViewParams
    {
        Matrix4f projection_matrix, view_matrix;
        void load(GLuint program, const Matrix4f& model_matrix)
        {
            Matrix4f modelview_matrix = view_matrix * model_matrix;
            Matrix4f PVM_matrix = projection_matrix * modelview_matrix;
            glUniformMatrix4fv(glGetUniformLocation(program, "PVM_matrix"), 1, GL_FALSE, (GLfloat *)&PVM_matrix);
            glUniformMatrix4fv(glGetUniformLocation(program, "modelview_matrix"), 1, GL_FALSE, (GLfloat *)&modelview_matrix);
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
    };
    ViewParams view;
    std::vector<LightParams> lights;
    std::vector<ModelParams> models;
public:
};

}
