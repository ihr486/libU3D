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

GLuint compile_shader(GLenum type, const char *src)
{
    GLuint shader = glCreateShader(type);

    glShaderSource(shader, 1, &src, NULL);
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

namespace
{
struct FormatBuffer
{
    char buf[32768];
    int position;
    FormatBuffer() : position(0) {}
    void print(const char *format, ...)
    {
        va_list ap;
        va_start(ap, format);
        int n = vsprintf(buf + position, format, ap);
        if(n >= 0) {
            position += n;
        }
        va_end(ap);
    }
};
}

ShaderGroup *LitTextureShader::create_shader_group(const Material* material)
{
    GLuint fragment_shader;
    {
        FormatBuffer fs;
        fs.print("#version 110\n"
                 "varying vec4 fragment_color;\n");
        for(int i = 0; i < 8; i++) {
            if(shader_channels & (1 << i)) {
                fs.print("uniform sampler2D texture%d;\n", i);
                fs.print("varying vec2 texcoord%d;\n", i);
            }
        }
        fs.print("void main() {\n");
        for(int i = 0; i < 8; i++) {
            if(i == 0) {
                fs.print("\tvec4 layer0_in = fragment_color;\n");
            } else {
                fs.print("\tvec4 layer%d_in = layer%d_out;\n", i, i - 1);
            }
            if(shader_channels & (1 << i)) {
                fs.print("\tvec4 layer%d_tex = texture2D(texture%d, texcoord%d);\n", i, i, i);
            }
            if(i == 7) {
                fs.print("\tgl_FragColor = ");
            } else {
                fs.print("\tvec4 layer%d_out = ", i);
            }
            if(shader_channels & (1 << i)) {
                switch(texinfos[i].blend_function) {
                case MULTIPLY:
                    fs.print("layer%d_in * layer%d_tex;\n", i, i);
                    break;
                case ADD:
                    fs.print("vec4(layer%d_in.rgb + layer%d_tex.rgb, layer%d_in.a * layer%d_tex.a);\n", i, i, i, i);
                    break;
                case REPLACE:
                    fs.print("layer%d_tex;\n", i);
                    break;
                case BLEND:
                    fs.print("vec4(mix(layer%d_in.rgb, layer%d_tex.rgb, layer%d_tex.a), layer%d_in.a * layer%d_tex.a);\n", i, i, i, i, i);
                    break;
                }
            } else {
                fs.print("layer%d_in;\n", i);
            }
        }
        fs.print("}\n");
        fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fs.buf);
    }

    const char vs_header[] =
        "#version 110\n"
        "attribute vec4 vertex_diffuse, vertex_specular;\n"
        "attribute vec4 vertex_position, vertex_normal;\n"
        "varying vec4 fragment_color;\n"
        "uniform mat4 PVM_matrix, modelview_matrix, normal_matrix;\n"
        "uniform vec4 material_diffuse, material_specular;\n"
        "uniform vec4 material_ambient, material_emissive;\n"
        "uniform float material_reflectivity, material_opacity;\n";

    GLuint ambient_shader;
    {
        FormatBuffer vsa;
        vsa.print("%s", vs_header);
        vsa.print("uniform vec4 light_color;\n");
        for(int i = 0; i < 8; i++) {
            if(shader_channels & (1 << i)) {
                vsa.print("attribute vec2 vertex_texcoord%d;\n", i);
                vsa.print("varying vec2 texcoord%d;\n", i);
            }
        }
        vsa.print("void main() {\n"
                     "\tvec4 ambient = light_color * material_ambient;\n"
                     "\tvec4 emissive = material_emissive;\n"
                     "\tfragment_color = ambient + emissive;\n");
        for(int i = 0; i < 8; i++) {
            if(shader_channels & (1 << i)) {
                vsa.print("\ttexcoord%d = vertex_texcoord%d * vec2(1.0, -1.0);\n", i, i);
            }
        }
        vsa.print("\tgl_Position = PVM_matrix * vertex_position;\n"
                     "}\n");
        ambient_shader = compile_shader(GL_VERTEX_SHADER, vsa.buf);
    }

    GLuint directional_shader;
    {
        FormatBuffer vsd;
        vsd.print("%s", vs_header);
        vsd.print("uniform vec4 light_color;\n"
                  "uniform vec4 light_direction;\n"
                  "uniform float light_intensity;\n");
        for(int i = 0; i < 8; i++) {
            if(shader_channels & (1 << i)) {
                vsd.print("attribute vec2 vertex_texcoord%d;\n", i);
                vsd.print("varying vec2 texcoord%d;\n", i);
            }
        }
        vsd.print("void main() {\n"
                  "\tvec4 viewspace_normal = normalize(normal_matrix * vertex_normal);\n"
                  "\tvec4 viewspace_position = modelview_matrix * vertex_position;\n"
                  "\tvec4 viewspace_incidence = light_direction;\n"
                  "\tvec4 viewspace_camera = normalize(-viewspace_position);\n");
        if(attributes & USE_VERTEX_COLOR) {
            vsd.print("\tvec4 diffuse = light_color * vertex_diffuse * max(0.0, dot(viewspace_normal, -viewspace_incidence));\n"
                      "\tvec4 specular = light_color * vertex_specular * pow(max(0.0, dot(viewspace_camera, reflect(viewspace_incidence, viewspace_normal))), material_reflectivity);\n");
        } else {
            vsd.print("\tvec4 diffuse = light_color * material_diffuse * max(0.0, dot(viewspace_normal, -viewspace_incidence));\n"
                      "\tvec4 specular = light_color * material_specular * pow(max(0.0, dot(viewspace_camera, reflect(viewspace_incidence, viewspace_normal))), material_reflectivity);\n");
        }
        vsd.print("\tvec4 ambient = light_color * material_ambient;\n"
                  "\tvec4 emissive = material_emissive;\n"
                  "\tfragment_color = light_intensity * (diffuse + specular + ambient) + emissive;\n"
                  "\tgl_Position = PVM_matrix * vertex_position;\n");
        for(int i = 0; i < 8; i++) {
            if(shader_channels & (1 << i)) {
                vsd.print("\ttexcoord%d = vertex_texcoord%d * vec2(1.0, -1.0);\n", i, i);
            }
        }
        vsd.print("}\n");
        directional_shader = compile_shader(GL_VERTEX_SHADER, vsd.buf);
    }

    GLuint point_shader;
    {
        FormatBuffer vsp;
        vsp.print("%s", vs_header);
        vsp.print("uniform vec4 light_color;\n"
                  "uniform vec4 light_position;\n"
                  "uniform float light_att0, light_att1, light_att2, light_intensity;\n");
        for(int i = 0; i < 8; i++) {
            if(shader_channels & (1 << i)) {
                vsp.print("attribute vec2 vertex_texcoord%d;\n", i);
                vsp.print("varying vec2 texcoord%d;\n", i);
            }
        }
        vsp.print("void main() {\n"
                  "\tvec4 viewspace_normal = normalize(normal_matrix * vertex_normal);\n"
                  "\tvec4 viewspace_position = modelview_matrix * vertex_position;\n"
                  "\tvec4 viewspace_incidence = normalize(viewspace_position - light_position);\n"
                  "\tvec4 viewspace_camera = normalize(-viewspace_position);\n");
        if(attributes & USE_VERTEX_COLOR) {
            vsp.print("\tvec4 diffuse = light_color * vertex_diffuse * max(0.0, dot(viewspace_normal, -viewspace_incidence));\n"
                      "\tvec4 specular = light_color * vertex_specular * pow(max(0.0, dot(viewspace_camera, reflect(viewspace_incidence, viewspace_normal))), material_reflectivity);\n");
        } else {
            vsp.print("\tvec4 diffuse = light_color * material_diffuse * max(0.0, dot(viewspace_normal, -viewspace_incidence));\n"
                      "\tvec4 specular = light_color * material_specular * pow(max(0.0, dot(viewspace_camera, reflect(viewspace_incidence, viewspace_normal))), material_reflectivity);\n");
        }
        vsp.print("\tvec4 ambient = light_color * material_ambient;\n"
                  "\tvec4 emissive = material_emissive;\n"
                  "\tfloat viewspace_light_distance = length(viewspace_position - light_position);\n"
                  "\tfloat attenuation = light_att0 + light_att1 * viewspace_light_distance + light_att2 * viewspace_light_distance * viewspace_light_distance;\n");
        for(int i = 0; i < 8; i++) {
            if(shader_channels & (1 << i)) {
                vsp.print("\ttexcoord%d = vertex_texcoord%d * vec2(1.0, -1.0);\n", i, i);
            }
        }
        vsp.print("\tfragment_color = light_intensity * ((diffuse + specular) / attenuation) + ambient + emissive;\n"
                  "\tgl_Position = PVM_matrix * vertex_position;\n"
                  "}\n");
        point_shader = compile_shader(GL_VERTEX_SHADER, vsp.buf);
    }

    GLuint spot_shader;
    {
        FormatBuffer vss;
        vss.print("%s", vs_header);
        vss.print("uniform vec4 light_color;\n"
                  "uniform vec4 light_position, light_direction;\n"
                  "uniform float light_spot_angle, light_exponent;\n"
                  "uniform float light_intensity, light_att0, light_att1, light_att2;\n");
        for(int i = 0; i < 8; i++) {
            if(shader_channels & (1 << i)) {
                vss.print("attribute vec2 vertex_texcoord%d;\n", i);
                vss.print("varying vec2 texcoord%d;\n", i);
            }
        }
        vss.print("void main() {\n"
                  "\tvec4 viewspace_normal = normalize(normal_matrix * vertex_normal);\n"
                  "\tvec4 viewspace_position = modelview_matrix * vertex_position;\n"
                  "\tvec4 viewspace_incidence = normalize(viewspace_position - light_position);\n"
                  "\tvec4 viewspace_camera = normalize(-viewspace_position);\n"
                  "\tfloat viewspace_light_distance = length(viewspace_position - light_position);\n"
                  "\tfloat attenuation = light_att0 + light_att1 * viewspace_light_distance + light_att2 * viewspace_light_distance * viewspace_light_distance;\n"
                  "\tfloat spot_attenuation = pow(dot(light_direction, viewspace_incidence), light_exponent);\n");
        if(attributes & USE_VERTEX_COLOR) {
            vss.print("\tvec4 diffuse = light_color * vertex_diffuse * max(0.0, dot(viewspace_normal, -viewspace_incidence));\n"
                      "\tvec4 specular = light_color * vertex_specular * pow(max(0.0, dot(viewspace_camera, reflect(viewspace_incidence, viewspace_normal))), material_reflectivity);\n");
        } else {
            vss.print("\tvec4 diffuse = light_color * material_diffuse * max(0.0, dot(viewspace_normal, -viewspace_incidence));\n"
                      "\tvec4 specular = light_color * material_specular * pow(max(0.0, dot(viewspace_camera, reflect(viewspace_incidence, viewspace_normal))), material_reflectivity);\n");
        }
        vss.print("\tvec4 ambient = light_color * material_ambient;\n"
                  "\tvec4 emissive = material_emissive;\n");
        for(int i = 0; i < 8; i++) {
            if(shader_channels & (1 << i)) {
                vss.print("\ttexcoord%d = vertex_texcoord%d * vec2(1.0, -1.0);\n", i, i);
            }
        }
        vss.print("\tfragment_color = light_intensity * (spot_attenuation * (diffuse + specular) / attenuation) + ambient + emissive;\n"
                  "\tgl_Position = PVM_matrix * vertex_position;\n"
                  "}\n");
        spot_shader = compile_shader(GL_VERTEX_SHADER, vss.buf);
    }

    ShaderGroup *group = new ShaderGroup();

    /*FILE *fs = fopen("common.frag", "r");
    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fs);
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

    group->ambient_program = link_program(ambient_shader, fragment_shader);
    group->directional_program = link_program(directional_shader, fragment_shader);
    group->point_program = link_program(point_shader, fragment_shader);
    group->spot_program = link_program(spot_shader, fragment_shader);
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
