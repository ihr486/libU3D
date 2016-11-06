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

#include "u3d_internal.hh"

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
            U3D_WARNING << log_msg << std::endl;
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
            U3D_WARNING << log_msg << std::endl;
            delete[] log_msg;
        }
    }

    return program;
}

}

namespace U3D
{

ShaderGroup *LitTextureShader::create_shader_group(const Material* material)
{
    FILE *fs = fopen("common.frag", "wb+");

    fprintf(fs, "#version 110\n"
                "varying vec4 fragment_color;\n");
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
            switch(texinfos[i].blend_function) {
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
    rewind(fs);
    GLuint common_fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fs);
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

    FILE *vsa = fopen("ambient.vert", "wb+");
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
                 "\tfragment_color = ambient + emissive;\n");
    for(int i = 0; i < 8; i++) {
        if(shader_channels & (1 << i)) {
            fprintf(vsa, "\ttexcoord%d = vertex_texcoord%d * vec2(1.0, -1.0);\n", i, i);
        }
    }
    fprintf(vsa, "\tgl_Position = PVM_matrix * vertex_position;\n"
                 "}\n");
    rewind(vsa);
    GLuint ambient_shader = compile_shader(GL_VERTEX_SHADER, vsa);
    fclose(vsa);

    FILE *vsd = fopen("directional.vert", "wb+");
    fprintf(vsd, "%s", vs_header);
    fprintf(vsd, "uniform vec4 light_color;\n"
                 "uniform vec4 light_direction;\n"
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
                 "\tvec4 viewspace_incidence = light_direction;\n"
                 "\tvec4 viewspace_camera = normalize(-viewspace_position);\n");
    if(attributes & USE_VERTEX_COLOR) {
        fprintf(vsd, "\tvec4 diffuse = light_color * vertex_diffuse * max(0.0, dot(viewspace_normal, -viewspace_incidence));\n"
                     "\tvec4 specular = light_color * vertex_specular * pow(max(0.0, dot(viewspace_camera, reflect(viewspace_incidence, viewspace_normal))), material_reflectivity);\n");
    } else {
        fprintf(vsd, "\tvec4 diffuse = light_color * material_diffuse * max(0.0, dot(viewspace_normal, -viewspace_incidence));\n"
                     "\tvec4 specular = light_color * material_specular * pow(max(0.0, dot(viewspace_camera, reflect(viewspace_incidence, viewspace_normal))), material_reflectivity);\n");
    }
    fprintf(vsd, "\tvec4 ambient = light_color * material_ambient;\n"
                 "\tvec4 emissive = material_emissive;\n"
                 "\tfragment_color = light_intensity * (diffuse + specular + ambient) + emissive;\n"
                 "\tgl_Position = PVM_matrix * vertex_position;\n");
    for(int i = 0; i < 8; i++) {
        if(shader_channels & (1 << i)) {
            fprintf(vsd, "\ttexcoord%d = vertex_texcoord%d * vec2(1.0, -1.0);\n", i, i);
        }
    }
    fprintf(vsd, "}\n");
    rewind(vsd);
    GLuint directional_shader = compile_shader(GL_VERTEX_SHADER, vsd);
    fclose(vsd);

    FILE *vsp = fopen("point.vert", "wb+");
    fprintf(vsp, "%s", vs_header);
    fprintf(vsp, "uniform vec4 light_color;\n"
                 "uniform vec4 light_position;\n"
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
                 "\tvec4 viewspace_incidence = normalize(viewspace_position - light_position);\n"
                 "\tvec4 viewspace_camera = normalize(-viewspace_position);\n");
    if(attributes & USE_VERTEX_COLOR) {
        fprintf(vsp, "\tvec4 diffuse = light_color * vertex_diffuse * max(0.0, dot(viewspace_normal, -viewspace_incidence));\n"
                     "\tvec4 specular = light_color * vertex_specular * pow(max(0.0, dot(viewspace_camera, reflect(viewspace_incidence, viewspace_normal))), material_reflectivity);\n");
    } else {
        fprintf(vsp, "\tvec4 diffuse = light_color * material_diffuse * max(0.0, dot(viewspace_normal, -viewspace_incidence));\n"
                     "\tvec4 specular = light_color * material_specular * pow(max(0.0, dot(viewspace_camera, reflect(viewspace_incidence, viewspace_normal))), material_reflectivity);\n");
    }
    fprintf(vsp, "\tvec4 ambient = light_color * material_ambient;\n"
                 "\tvec4 emissive = material_emissive;\n"
                 "\tfloat viewspace_light_distance = length(viewspace_position - light_position);\n"
                 "\tfloat attenuation = light_att0 + light_att1 * viewspace_light_distance + light_att2 * viewspace_light_distance * viewspace_light_distance;\n");
    for(int i = 0; i < 8; i++) {
        if(shader_channels & (1 << i)) {
            fprintf(vsp, "\ttexcoord%d = vertex_texcoord%d * vec2(1.0, -1.0);\n", i, i);
        }
    }
    fprintf(vsp, "\tfragment_color = light_intensity * ((diffuse + specular) / attenuation) + ambient + emissive;\n"
                 "\tgl_Position = PVM_matrix * vertex_position;\n"
                 "}\n");
    rewind(vsp);
    GLuint point_shader = compile_shader(GL_VERTEX_SHADER, vsp);
    fclose(vsp);

    FILE *vss = fopen("spot.vert", "wb+");
    fprintf(vss, "%s", vs_header);
    fprintf(vss, "uniform vec4 light_color;\n"
                 "uniform vec4 light_position, light_direction;\n"
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
                 "\tvec4 viewspace_incidence = normalize(viewspace_position - light_position);\n"
                 "\tvec4 viewspace_camera = normalize(-viewspace_position);\n"
                 "\tfloat viewspace_light_distance = length(viewspace_position - light_position);\n"
                 "\tfloat attenuation = light_att0 + light_att1 * viewspace_light_distance + light_att2 * viewspace_light_distance * viewspace_light_distance;\n"
                 "\tfloat spot_attenuation = pow(dot(light_direction, viewspace_incidence), light_exponent);\n");
    if(attributes & USE_VERTEX_COLOR) {
        fprintf(vss, "\tvec4 diffuse = light_color * vertex_diffuse * max(0.0, dot(viewspace_normal, -viewspace_incidence));\n"
                     "\tvec4 specular = light_color * vertex_specular * pow(max(0.0, dot(viewspace_camera, reflect(viewspace_incidence, viewspace_normal))), material_reflectivity);\n");
    } else {
        fprintf(vss, "\tvec4 diffuse = light_color * material_diffuse * max(0.0, dot(viewspace_normal, -viewspace_incidence));\n"
                     "\tvec4 specular = light_color * material_specular * pow(max(0.0, dot(viewspace_camera, reflect(viewspace_incidence, viewspace_normal))), material_reflectivity);\n");
    }
    fprintf(vss, "\tvec4 ambient = light_color * material_ambient;\n"
                 "\tvec4 emissive = material_emissive;\n");
    for(int i = 0; i < 8; i++) {
        if(shader_channels & (1 << i)) {
            fprintf(vss, "\ttexcoord%d = vertex_texcoord%d * vec2(1.0, -1.0);\n", i, i);
        }
    }
    fprintf(vss, "\tfragment_color = light_intensity * (spot_attenuation * (diffuse + specular) / attenuation) + ambient + emissive;\n"
                 "\tgl_Position = PVM_matrix * vertex_position;\n"
                 "}\n");
    rewind(vss);
    GLuint spot_shader = compile_shader(GL_VERTEX_SHADER, vss);
    fclose(vss);

    ShaderGroup *group = new ShaderGroup();

    /*FILE *fs = fopen("common.frag", "r");
    GLuint common_fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fs);
    fclose(fs);
    FILE *vsa = fopen("ambient.vert", "r");
    GLuint ambient_shader = compile_shader(GL_VERTEX_SHADER, vsa);
    fclose(vsa);
    FILE *vsd = fopen("directional.vert", "r");
    GLuint directional_shader = compile_shader(GL_VERTEX_SHADER, vsd);
    fclose(vsd);
    FILE *vsp = fopen("point.vert", "r");
    GLuint point_shader = compile_shader(GL_VERTEX_SHADER, vsp);
    fclose(vsp);
    FILE *vss = fopen("spot.vert", "r");
    GLuint spot_shader = compile_shader(GL_VERTEX_SHADER, vss);
    fclose(vss);*/

    group->ambient_program = link_program(ambient_shader, common_fragment_shader);
    group->directional_program = link_program(directional_shader, common_fragment_shader);
    group->point_program = link_program(point_shader, common_fragment_shader);
    group->spot_program = link_program(spot_shader, common_fragment_shader);
    group->material.configure(material);
    group->shader_channels = shader_channels & 0xFF;
    for(int i = 0; i < 8; i++) {
        if(shader_channels & (1 << i)) {
            group->texture_names[i] = texinfos[i].name;
        }
    }

    return group;
}

}
