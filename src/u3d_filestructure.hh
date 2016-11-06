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
class FileStructure
{
    std::map<std::string, ModelResource *> models;
    std::map<std::string, LightResource *> lights;
    std::map<std::string, ViewResource *> views;
    std::map<std::string, Texture *> textures;
    std::map<std::string, LitTextureShader *> shaders;
    std::map<std::string, Material *> materials;
    std::map<std::string, Node *> nodes;
    BitStreamReader reader;
public:
    FileStructure(const std::string& filename);
    ~FileStructure() {
        for(std::map<std::string, ModelResource *>::iterator i = models.begin(); i != models.end(); i++) delete i->second;
        for(std::map<std::string, LightResource *>::iterator i = lights.begin(); i != lights.end(); i++) delete i->second;
        for(std::map<std::string, ViewResource *>::iterator i = views.begin(); i != views.end(); i++) delete i->second;
        for(std::map<std::string, Texture *>::iterator i = textures.begin(); i != textures.end(); i++) delete i->second;
        for(std::map<std::string, LitTextureShader *>::iterator i = shaders.begin(); i != shaders.end(); i++) delete i->second;
        for(std::map<std::string, Material *>::iterator i = materials.begin(); i != materials.end(); i++) delete i->second;
        for(std::map<std::string, Node *>::iterator i = nodes.begin(); i != nodes.end(); i++) delete i->second;
    }
    Node *get_node(const std::string& name) {
        std::map<std::string, Node *>::iterator i = nodes.find(name);
        if(i != nodes.end()) {
            return i->second;
        } else {
            return NULL;
        }
    }
    View *get_view(const std::string& name) {
        std::map<std::string, Node *>::iterator i = nodes.find(name);
        if(i != nodes.end()) {
            return dynamic_cast<View *>(nodes[name]);
        } else {
            return NULL;
        }
    }
    View *get_first_view() {
        for(std::map<std::string, Node *>::iterator i = nodes.begin(); i != nodes.end(); i++) {
            View *view = dynamic_cast<View *>(i->second);
            if(view != NULL) return view;
        }
        return NULL;
    }
    bool get_world_transform(Matrix4f *mat, const Node *node, const Node *root) {
        if(node == root) {
            return true;
        } else {
            for(std::vector<Node::Parent>::const_iterator i = node->parents.begin(); i != node->parents.end(); i++) {
                Node *parent = nodes[i->name];
                if(get_world_transform(mat, parent, root)) {
                    U3D_LOG << "Parent = " << i->name << std::endl;
                    U3D_LOG << "Transform = " << i->transform << std::endl;
                    *mat *= i->transform;
                    return true;
                }
            }
        }
        return false;
    }
    GraphicsContext *create_context();
    SceneGraph *create_scenegraph(const View *view, int pass_index);
};
}
