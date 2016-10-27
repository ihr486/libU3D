#include "shader.hpp"
#include "util.hpp"

#include <stdio.h>
#include <stdlib.h>

namespace
{

GLuint compile_shader(GLenum type, FILE *fp)
{
    GLuint shader = glCreateShader(type);

    char buf[32768];
    size_t src_length = fread(buf, 1, sizeof(buf) - 1, fp);
    if(src_length >= sizeof(buf) - 1) {
        U3D_WARNING << "Shader source might be truncated." << std::endl;
    }
    buf[src_length] = 0;
    const char *source_list[1] = {buf};
    glShaderSource(shader, 1, source_list, NULL);
    glCompileShader(shader);

    GLint result, log_length;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    if(result == GL_FALSE) {
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
        if(log_length > 0) {
            char *log_msg = new char[log_length];
            glGetShaderInfoLog(shader, log_length, NULL, log_msg);
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

LitTextureShader::ShaderGroup *LitTextureShader::create_shader_group()
{
    FILE *fs = fopen("common.frag", "w");

    fprintf(fs, "#version 120\nvarying vec4 fragment_color;\n");
    for(int i = 0; i < 8; i++) {
        if(shader_channels & (1 << i)) {
            fprintf(fs, "uniform sampler2D texture%d;\n", i);
            fprintf(fs, "varying vec2 texcoord%d;\n", i);
        }
    }
    fprintf(fs, "void main() {\n");
    for(int i = 0; i < 8; i++) {
        if(i == 0) {
            fprintf(fs, "\tvec4 layer0_in = fragment_color;\n");
        } else {
            fprintf(fs, "\tvec4 layer%d_in = layer%d_out;\n", i, i - 1);
        }
        if(shader_channels & (1 << i)) {
            fprintf(fs, "\tvec4 layer%d_tex = texture(texture%d, texcoord%d);\n", i, i, i);
        }
        if(i == 7) {
            fprintf(fs, "\tgl_FragColor = ");
        } else {
            fprintf(fs, "\tvec4 layer%d_out = ", i);
        }
        if(shader_channels & (1 << i)) {
            switch(texinfos[i].mode) {
            case MULTIPLY:
                fprintf(fs, "layer%d_in * layer%d_tex;\n", i, i);
                break;
            case ADD:
                fprintf(fs, "vec4(layer%d_in.rgb + layer%d_tex.rgb, layer%d_in.a * layer%d_tex.a);\n", i, i, i, i);
                break;
            case REPLACE:
                fprintf(fs, "layer%d_tex;\n", i);
                break;
            case BLEND:
                fprintf(fs, "vec4(mix(layer%d_in.rgb, layer%d_tex.rgb, layer%d_tex.a), layer%d_in.a * layer%d_tex.a);\n", i, i, i, i, i);
                break;
            }
        } else {
            fprintf(fs, "layer%d_in;\n", i);
        }
    }
    fprintf(fs, "}\n");
    //GLuint common_fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fs);
    fclose(fs);

    FILE *vsd = fopen("directional.vert", "w");
    fprintf(vsd, "#version 120\n"
                 "attribute vec4 vertex_diffuse, vertex_specular;\n"
                 "attribute vec4 vertex_position, vertex_normal;\n"
                 "varying vec4 fragment_color;\n"
                 "uniform mat4 PVM_matrix, modelview_matrix, normal_matrix;\n"
                 "uniform vec4 material_diffuse, material_specular;\n"
                 "uniform vec4 material_ambient, material_emissive;\n"
                 "uniform float material_reflectivity, material_opacity;\n");
    fprintf(vsd, "uniform vec4 light_color;\n"
                 "uniform mat4 light_transform;\n");
    for(int i = 0; i < 8; i++) {
        if(shader_channels & (1 << i)) {
            fprintf(vsd, "\tvarying vec2 texcoord%d;\n", i);
        }
    }
    fprintf(vsd, "void main() {\n"
                 "\tviewspace_normal = normalize(normal_matrix * vertex_normal);\n"
                 "\tviewspace_position = modelview_matrix * vertex_position;\n"
                 "\tviewspace_camera = normalize(-viewspace_position);\n"
                 "\tdiffuse = light_color * material_diffuse * max(0.0, dot(viewspace_normal, viewspace_incidence));\n"
                 "\tspecular = light_color * material_specular * pow(max(0.0, dot(viewspace_camera, reflect(viewspace_incidence, viewspace_normal))), material_reflectivity);\n"
                 "\tambient = light_color * material_ambient;\n"
                 "\temissive = material_emissive;\n"
                 "\tfragment_color = light_intensity * (diffuse + specular + ambient) + emissive;\n"
                 "\tgl_Position = PVM_matrix * vertex_position;\n");
    for(int i = 0; i < 8; i++) {
        if(shader_channels & (1 << i)) {
            fprintf(vsd, "\ttexcoord%d = vertex_texcoord%d;\n", i, i);
        }
    }
    fprintf(vsd, "}\n");
    fclose(vsd);

    //ShaderGroup *group = new ShaderGroup();

    return NULL;
}

}
