#include "shader.hpp"

namespace
{

GLuint compile_shader(GLenum type, const std::string& source)
{
    GLuint shader = glCreateShader(type);

    const char *source_list[1] = {source.c_str()};
    glShaderSource(shader, 1, source_list, NULL);
    glCompileShader(shader);

    GLint result, length;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    if(result == GL_FALSE) {
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        if(length > 0) {
            char *log_msg = new char[length];
            glGetShaderInfoLog(shader, length, NULL, log_msg);
            delete[] log_msg;
        }
    }

    return shader;
}

GLuint link_program(GLuint vertex_shader, GLuint fragment_shader)
{
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    GLint result, length;
    glGetProgramiv(program, GL_LINK_STATUS, &result);
    if(result == GL_FALSE) {
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        if(length > 0) {
            char *log_msg = new char[length];
            glGetProgramInfoLog(program, length, NULL, log_msg);
            delete[] log_msg;
        }
    }

    return program;
}

}

namespace U3D
{

ShaderGroup *create_shader_group()
{
    std::ostringstream fragment_source;
    fragment_source <<
        "#version 120\n"
        "varying vec4 fragment_color;\n"
    ;
    for(int i = 0; i < 8; i++) {
        if(shader_channels & (1 << i)) {
            fragment_source <<
                "uniform sampler2D texture" << i << "\n"
            ;
        }
    }
    fragment_source <<
        "void main() {\n"
    ;
    for(int i = 0; i < 8; i++) {
        if(i == 0) {
            fragment_source << "\tvec4 layer0_in = fragment_color;\n";
        } else {
            fragment_source << "\tvec4 layer" << i << "_in = layer" << i - 1 << "_out;\n";
        }
        if(shader_channels & (1 << i)) {
            fragment_source << "\tvec4 layer" << i << "_tex = sampler2D(texture" << i << ");\n";
        }
        if(i == 7) {
            fragment_source << "\tgl_FragColor = ";
        } else {
            fragment_source << "\tvec4 layer" << i << "_out = ";
        }
        if(shader_channels & (1 << i)) {
            switch(texinfos[i].mode) {
            case MULTIPLY:
                fragment_source <<
                    "layer" << i << "_in * layer" << i << "_tex;\n"
                ;
                break;
            case ADD:
                fragment_source <<
                    "vec4(layer" << i << "_in.rgb + layer" << i << "_tex.rgb, layer" << i << "_in.a * layer" << i << "_tex.a);\n"
                ;
                break;
            case REPLACE:
                fragment_source <<
                    "layer" << i << "_tex;\n"
                ;
                break;
            case BLEND:
                fragment_source <<
                    "vec4(mix(layer" << i << "_in.rgb, layer" << i << "_tex.rgb, layer" << i << "_tex.a), layer" << i << "_in.a * layer" << i << "_tex.a);\n"
                ;
                break;
            }
        } else {
            fragment_source << "layer" << i << "_in;\n"; 
        }
    }
    fragment_source << "}\n";
    GLuint common_fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_source.str());

    ShaderGroup *group = new ShaderGroup();
    std::ostringstream vertex_source;
    vertex_source <<
    "#version 120\n"
    "attribute vec4 vertex_position, vertex_normal;\n"
    "attribute vec4 vertex_diffuse, vertex_specular;\n"
    "varying vec4 fragment_color;\n"
    "uniform mat4 PVM_matrix, normal_matrix;\n"
    "uniform vec3 light_dir;\n"
    "uniform float light_intensity;\n"
    "uniform vec3 light_color;\n";
    vertex_source << 
    "void main() {\n"
    "    gl_Position = PVM_matrix * vertex_position;\n"
    "    view_normal = normal_matrix * vertex_normal;\n"
    "    fragment_color = vertex_diffuse;\n"
    "}\n";
}

}
