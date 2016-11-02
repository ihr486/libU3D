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

ShaderGroup *LitTextureShader::create_shader_group()
{
    FILE *fs = fopen("common.frag", "w");

    fprintf(fs, "#version 110\nvarying vec4 fragment_color;\n");
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
            fprintf(fs, "\tvec4 layer%d_tex = texture2D(texture%d, texcoord%d);\n", i, i, i);
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

    const char vs_header[] =
        "#version 110\n"
        "attribute vec4 vertex_diffuse, vertex_specular;\n"
        "attribute vec4 vertex_position, vertex_normal;\n"
        "varying vec4 fragment_color;\n"
        "uniform mat4 PVM_matrix, modelview_matrix, normal_matrix;\n"
        "uniform vec4 material_diffuse, material_specular;\n"
        "uniform vec4 material_ambient, material_emissive;\n"
        "uniform float material_reflectivity, material_opacity;\n";

    FILE *vsa = fopen("ambient.vert", "w");
    fprintf(vsa, "%s", vs_header);
    fprintf(vsa, "uniform vec4 light_color;\n");
    for(int i = 0; i < 8; i++) {
        if(shader_channels & (1 << i)) {
            fprintf(vsa, "attribute vec2 vertex_texcoord%d;\n", i);
            fprintf(vsa, "varying vec2 texcoord%d;\n", i);
        }
    }
    fprintf(vsa, "void main() {\n"
                 "\tvec4 ambient = light_color * material_ambient;\n"
                 "\tvec4 emissive = material_emissive;\n"
                 "\tfragment_color = ambient + emissive;\n"
                 "\tgl_Position = PVM_matrix * vertex_position;\n");
    for(int i = 0; i < 8; i++) {
        if(shader_channels & (1 << i)) {
            fprintf(vsa, "\ttexcoord%d = vertex_texcoord%d;\n", i, i);
        }
    }
    fprintf(vsa, "}\n");
    fclose(vsa);

    FILE *vsd = fopen("directional.vert", "w");
    fprintf(vsd, "%s", vs_header);
    fprintf(vsd, "uniform vec4 light_color;\n"
                 "uniform mat4 light_transform;\n"
                 "uniform float light_intensity;\n");
    for(int i = 0; i < 8; i++) {
        if(shader_channels & (1 << i)) {
            fprintf(vsd, "attribute vec2 vertex_texcoord%d;\n", i);
            fprintf(vsd, "varying vec2 texcoord%d;\n", i);
        }
    }
    fprintf(vsd, "void main() {\n"
                 "\tvec4 viewspace_normal = normalize(normal_matrix * vertex_normal);\n"
                 "\tvec4 viewspace_position = modelview_matrix * vertex_position;\n"
                 "\tvec4 viewspace_incidence = light_transform * vec4(0.0, 0.0, -1.0, 0.0);\n"
                 "\tvec4 viewspace_camera = normalize(-viewspace_position);\n"
                 "\tvec4 diffuse = light_color * material_diffuse * max(0.0, dot(viewspace_normal, viewspace_incidence));\n"
                 "\tvec4 specular = light_color * material_specular * pow(max(0.0, dot(viewspace_camera, reflect(viewspace_incidence, viewspace_normal))), material_reflectivity);\n"
                 "\tvec4 ambient = light_color * material_ambient;\n"
                 "\tvec4 emissive = material_emissive;\n"
                 "\tfragment_color = light_intensity * (diffuse + specular + ambient) + emissive;\n"
                 "\tgl_Position = PVM_matrix * vertex_position;\n");
    for(int i = 0; i < 8; i++) {
        if(shader_channels & (1 << i)) {
            fprintf(vsd, "\ttexcoord%d = vertex_texcoord%d;\n", i, i);
        }
    }
    fprintf(vsd, "}\n");
    fclose(vsd);

    FILE *vsp = fopen("point.vert", "w");
    fprintf(vsp, "%s", vs_header);
    fprintf(vsp, "uniform vec4 light_color;\n"
                 "uniform mat4 light_transform;\n"
                 "uniform float light_att0, light_att1, light_att2, light_intensity;\n");
    for(int i = 0; i < 8; i++) {
        if(shader_channels & (1 << i)) {
            fprintf(vsp, "attribute vec2 vertex_texcoord%d;\n", i);
            fprintf(vsp, "varying vec2 texcoord%d;\n", i);
        }
    }
    fprintf(vsp, "void main() {\n"
                 "\tvec4 viewspace_normal = normalize(normal_matrix * vertex_normal);\n"
                 "\tvec4 viewspace_position = modelview_matrix * vertex_position;\n"
                 "\tvec4 viewspace_light_position = modelview_matrix * light_transform * vec4(0.0, 0.0, 0.0, 1.0);\n"
                 "\tvec4 viewspace_incidence = normalize(viewspace_position - viewspace_light_position);\n"
                 "\tvec4 viewspace_camera = normalize(-viewspace_position);\n"
                 "\tvec4 diffuse = light_color * material_diffuse * max(0.0, dot(viewspace_normal, viewspace_incidence));\n"
                 "\tvec4 specular = light_color * material_specular * pow(max(0.0, dot(viewspace_camera, reflect(viewspace_incidence, viewspace_normal))), material_reflectivity);\n"
                 "\tvec4 ambient = light_color * material_ambient;\n"
                 "\tvec4 emissive = material_emissive;\n"
                 "\tfloat viewspace_light_distance = length(viewspace_position - viewspace_light_position);\n"
                 "\tfloat attenuation = light_att0 + light_att1 * viewspace_light_distance + light_att2 * viewspace_light_distance * viewspace_light_distance;\n");
    for(int i = 0; i < 8; i++) {
        if(shader_channels & (1 << i)) {
            fprintf(vsp, "\ttexcoord%d = vertex_texcoord%d;\n", i);
        }
    }
    fprintf(vsp, "\tfragment_color = light_intensity * ((diffuse + specular) / attenuation + ambient) + emissive;\n"
                 "\tgl_Position = PVM_matrix * vertex_position;\n"
                 "}\n");
    fclose(vsp);

    FILE *vss = fopen("spot.vert", "w");
    fprintf(vss, "%s", vs_header);
    fprintf(vss, "uniform vec4 light_color;\n"
                 "uniform mat4 light_transform;\n"
                 "uniform float light_spot_angle, light_exponent;\n"
                 "uniform float light_intensity, light_att0, light_att1, light_att2;\n");
    for(int i = 0; i < 8; i++) {
        if(shader_channels & (1 << i)) {
            fprintf(vss, "attribute vec2 vertex_texcoord%d;\n", i);
            fprintf(vss, "varying vec2 texcoord%d;\n", i);
        }
    }
    fprintf(vss, "void main() {\n"
                 "\tvec4 viewspace_normal = normalize(normal_matrix * vertex_normal);\n"
                 "\tvec4 viewspace_position = modelview_matrix * vertex_position;\n"
                 "\tvec4 viewspace_light_position = modelview_matrix * light_transform * vec4(0.0, 0.0, 0.0, 1.0);\n"
                 "\tvec4 viewspace_incidence = normalize(viewspace_position - viewspace_light_position);\n"
                 "\tvec4 viewspace_camera = normalize(-viewspace_position);\n"
                 "\tfloat viewspace_light_distance = length(viewspace_position - viewspace_light_position);\n"
                 "\tfloat attenuation = light_att0 + light_att1 * viewspace_light_distance + light_att2 * viewspace_light_distance * viewspace_light_distance;\n"
                 "\tfloat spot_attenuation = pow(dot(normalize(light_transform * vec4(0.0, 0.0, -1.0, 0.0)), viewspace_incidence), light_exponent);\n"
                 "\tvec4 diffuse = light_color * material_diffuse * max(0.0, dot(viewspace_normal, viewspace_incidence));\n"
                 "\tvec4 specular = light_color * material_specular * pow(max(0.0, dot(viewspace_camera, reflect(viewspace_incidence, viewspace_normal))), material_reflectivity);\n"
                 "\tvec4 ambient = light_color * material_ambient;\n"
                 "\tvec4 emissive = material_emissive;\n");
    for(int i = 0; i < 8; i++) {
        if(shader_channels & (1 << i)) {
            fprintf(vss, "\ttexcoord%d = vertex_texcoord%d;\n", i, i);
        }
    }
    fprintf(vss, "\tfragment_color = light_intensity * (spot_attenuation * (diffuse + specular) / attenuation) + ambient + emissive;\n"
                 "\tgl_Position = PVM_matrix * vertex_position;\n"
                 "}\n");

    //ShaderGroup *group = new ShaderGroup();

    return NULL;
}

}
