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
class GraphicsContext
{
    std::map<std::string, ShaderGroup *> shader_groups;
    std::map<std::string, GLuint> textures;
    std::map<std::string, RenderGroup *> render_groups;
public:
    GraphicsContext() {}
    ~GraphicsContext() {
        for(std::map<std::string, ShaderGroup *>::iterator i = shader_groups.begin(); i != shader_groups.end(); i++) {
            delete i->second;
        }
        for(std::map<std::string, GLuint>::iterator i = textures.begin(); i != textures.end(); i++) {
            GLuint texture = i->second;
            glDeleteTextures(1, &texture);
        }
        for(std::map<std::string, RenderGroup *>::iterator i = render_groups.begin(); i != render_groups.end(); i++) {
            delete i->second;
        }
    }
    ShaderGroup *get_shader_group(const std::string& name)
    {
        std::map<std::string, ShaderGroup *>::iterator i = shader_groups.find(name);
        if(i != shader_groups.end()) {
            return i->second;
        } else {
            return NULL;
        }
    }
    GLuint get_texture(const std::string& name)
    {
        std::map<std::string, GLuint>::iterator i = textures.find(name);
        if(i != textures.end()) {
            return i->second;
        } else {
            return 0;
        }
    }
    RenderGroup *get_render_group(const std::string& name)
    {
        std::map<std::string, RenderGroup *>::iterator i = render_groups.find(name);
        if(i != render_groups.end()) {
            return i->second;
        } else {
            return 0;
        }
    }
    void add_shader_group(const std::string& name, ShaderGroup *shader_group)
    {
        shader_groups[name] = shader_group;
    }
    void add_texture(const std::string& name, GLuint texture)
    {
        textures[name] = texture;
    }
    void add_render_group(const std::string& name, RenderGroup *render_group)
    {
        render_groups[name] = render_group;
    }
};
}
