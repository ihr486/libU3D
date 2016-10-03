#include "shader.hpp"

namespace U3D
{

static GLuint create_program(const std::string& vertex_source, const std::string& fragment_source)
{
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    const char *vertex_source_list[1] = {vertex_source.c_str()};
    glShaderSource(vertex_shader, 1, vertex_source_list, NULL);
    glCompileShader(vertex_shader);

    GLint result, length;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &result);
    if(result == GL_FALSE) {
        glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &length);
        if(length > 0) {
            char *log_msg = new char[length];
            glGetShaderInfoLog(vertex_shader, length, NULL, log_msg);
            delete[] log_msg;
        }
    }

    const char *fragment_source_list[1] = {fragment_source.c_str()};
    glShaderSource(fragment_shader, 1, fragment_source_list, NULL);
    glCompileShader(fragment_shader);

    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &result);
    if(result == GL_FALSE) {
        glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &length);
        if(length > 0) {
            char *log_msg = new char[length];
            glGetShaderInfoLog(fragment_shader, length, NULL, log_msg);
            delete[] log_msg;
        }
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &result);
    if(result == GL_FALSE) {
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        if(length > 0) {
            char *log_msg = new char[length];
            glGetProgramInfoLog(program, length, NULL, log_msg);
            delete[] log_msg;
        }
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return program;
}

ShaderGroup *create_shader_group()
{
    ShaderGroup *group = new ShaderGroup();
    std::ostringstream vertex_source;
    vertex_source <<
    "#version 100 core\n"
    "attribute vec4 vertex_position, vertex_normal;\n"
    "attribute vec4 vertex_diffuse, vertex_specular;\n"
    "varying vec4 fragment_color;\n"
    "uniform mat4 PVM_matrix;\n"
    "uniform vec3 light_dir;\n"
    "uniform float light_intensity;\n"
    "uniform vec3 light_color;\n"
    "void main() {\n"
    "    gl_Position = PVM_matrix * vertex_position;\n"
    "    view_normal = PVM_matrix * vec4(vertex_normal.xyz, 0.0);\n"
    "    fragment_color = vertex_diffuse;\n"
    "}\n";
    std::ostringstream fragment_source;
    fragment_source <<
    "#version 100 core\n"
    "varying vec4 fragment_color;\n"
    "void main() {\n"
    "    gl_FragColor = fragment_color;\n"
    "}\n";
    group->directional_program = create_program(vertex_source.str(), fragment_source.str());
}

}
