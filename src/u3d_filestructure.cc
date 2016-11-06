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

namespace U3D
{
namespace
{
uint32_t read_modifier_count(BitStreamReader& reader)
{
    uint32_t attribute;
    reader >> attribute;
    if(attribute & 0x00000001) {
        reader.read<Vector3f>();
        reader.read<float>();
    } else if(attribute & 0x00000002) {
        reader.read<Vector3f>();
        reader.read<Vector3f>();
    }
    reader.align_to_word();
    return reader.read<uint32_t>();
}

Node *create_node_modifier_chain(BitStreamReader& reader)
{
    Node *head = NULL;
    uint32_t count = read_modifier_count(reader);
    for(unsigned int i = 0; i < count; i++) {
        BitStreamReader::SubBlock subblock(reader);
        std::string name = reader.read_str();
        switch(subblock.get_type()) {
        case 0xFFFFFF21:    //Group Node Block
            head = static_cast<Node *>(new Group(reader));
            std::fprintf(stderr, "\tGroup node \"%s\"\n", name.c_str());
            break;
        case 0xFFFFFF22:    //Model Node Block
            head = static_cast<Node *>(new Model(reader));
            std::fprintf(stderr, "\tModel \"%s\"\n", name.c_str());
            break;
        case 0xFFFFFF23:    //Light Node Block
            head = static_cast<Node *>(new Light(reader));
            std::fprintf(stderr, "\tLight \"%s\"\n", name.c_str());
            break;
        case 0xFFFFFF24:    //View Node Block
            head = static_cast<Node *>(new View(reader));
            std::fprintf(stderr, "\tView \"%s\"\n", name.c_str());
            break;
        case 0xFFFFFF45:    //Shading Modifier Block
            if(head != NULL) {
                Model *model = dynamic_cast<Model *>(head);
                if(model != NULL) {
                    model->add_shading_modifier(new Shading(reader));
                }
            }
            std::fprintf(stderr, "\tShading \"%s\"\n", name.c_str());
            break;
        default:
            std::fprintf(stderr, "Illegal modifier 0x%08X in a node modifier chain\n", subblock.get_type());
            return head;
        }
    }
    return head;
}

ModelResource *create_model_modifier_chain(BitStreamReader& reader)
{
    ModelResource *head = NULL;
    uint32_t count = read_modifier_count(reader);
    for(unsigned int i = 0; i < count; i++) {
        BitStreamReader::SubBlock subblock(reader);
        std::string name = reader.read_str();
        switch(subblock.get_type()) {
        case 0xFFFFFF31:    //CLOD Mesh Generator Declaration
            head = static_cast<ModelResource *>(new CLOD_Mesh(reader));
            std::fprintf(stderr, "\tCLOD Mesh \"%s\"\n", name.c_str());
            break;
        case 0xFFFFFF36:    //Point Set Declaration
            head = static_cast<ModelResource *>(new PointSet(reader));
            std::fprintf(stderr, "\tPoint Set \"%s\"\n", name.c_str());
            break;
        case 0xFFFFFF37:    //Line Set Declaration
            head = static_cast<ModelResource *>(new LineSet(reader));
            std::fprintf(stderr, "\tLine Set \"%s\"\n", name.c_str());
            break;
        case 0xFFFFFF42:    //Subdivision Modifier Block
        case 0xFFFFFF43:    //Animation Modifier Block
        case 0xFFFFFF44:    //Bone Weight Modifier Block
        case 0xFFFFFF46:    //CLOD Modifier Block
            std::fprintf(stderr, "Modifier Type 0x%08X is not implemented in the current version.\n", subblock.get_type());
            break;
        case 0xFFFFFF45:
            std::fprintf(stderr, "Shading Modifier \"%s\"\n", name.c_str());
            head->add_shading_modifier(new Shading(reader));
            break;
        default:
            std::fprintf(stderr, "Illegal modifier 0x%08X in an instance modifier chain\n", subblock.get_type());
            return head;
        }
    }
    return head;
}

Texture *create_texture_modifier_chain(BitStreamReader& reader)
{
    Texture *head = NULL;
    uint32_t count = read_modifier_count(reader);
    for(unsigned int i = 0; i < count; i++) {
        BitStreamReader::SubBlock subblock(reader);
        std::string name = reader.read_str();
        switch(subblock.get_type()) {
        case 0xFFFFFF55:    //Texture Declaration
            head = new Texture(reader);
            std::fprintf(stderr, "Texture Resource \"%s\"\n", name.c_str());
            break;
        default:
            std::fprintf(stderr, "Illegal modifier 0x%08X in an instance modifier chain\n", subblock.get_type());
            return head;
        }
    }
    return head;
}

void read_header_block(BitStreamReader& reader) {
    uint16_t major_version, minor_version;
    uint32_t profile_identifier;
    uint32_t declaration_size;
    uint64_t file_size;
    uint32_t character_encoding;
    double units_scaling_factor;
    reader >> major_version >> minor_version >> profile_identifier >> declaration_size >> file_size >> character_encoding;
    units_scaling_factor = (profile_identifier & 0x8) ? reader.read<double>() : 1;
    U3D_LOG << "Scaling factor = " << units_scaling_factor << std::endl;
}
}

FileStructure::FileStructure(const std::string& filename) : reader(filename)
{
    models[""] = new CLOD_Mesh();
    lights[""] = new LightResource();
    views[""] = new ViewResource();
    textures[""] = new Texture();
    shaders[""] = new LitTextureShader();
    materials[""] = new Material();
    nodes[""] = static_cast<Node *>(new Group());

    while(reader.open_block()) {
        std::string name;

        switch(reader.get_type()) {
        case 0x00443355:    //File Header Block
            read_header_block(reader);
            break;
        case 0xFFFFFF14:    //Modifier Chain Block
            name = reader.read_str();
            std::fprintf(stderr, "Modifier Chain \"%s\"\n", name.c_str());
            switch(reader.read<uint32_t>()) {
            case 0:
                nodes[name] = create_node_modifier_chain(reader);
                break;
            case 1:
                models[name] = create_model_modifier_chain(reader);
                break;
            case 2:
                textures[name] = create_texture_modifier_chain(reader);
                break;
            }
            break;
        case 0xFFFFFF15:    //Priority Update Block
            reader.read<uint32_t>();
            break;
        case 0xFFFFFF16:    //New Object Type Block
            std::fprintf(stderr, "New Object Type block is not supported in the current version.\n");
            return;
        case 0xFFFFFF51:    //Light Resource Block
            name = reader.read_str();
            lights[name] = new LightResource(reader);
            std::fprintf(stderr, "Light Resource \"%s\"\n", name.c_str());
            break;
        case 0xFFFFFF52:    //View Resource Block
            name = reader.read_str();
            views[name] = new ViewResource(reader);
            std::fprintf(stderr, "View Resource \"%s\"\n", name.c_str());
            break;
        case 0xFFFFFF53:    //Lit Texture Shader Block
            name = reader.read_str();
            shaders[name] = new LitTextureShader(reader);
            std::fprintf(stderr, "Lit Texture Shader Resource \"%s\"\n", name.c_str());
            break;
        case 0xFFFFFF54:    //Material Block
            name = reader.read_str();
            materials[name] = new Material(reader);
            std::fprintf(stderr, "Material \"%s\"\n", name.c_str());
            break;
        case 0xFFFFFF55:    //Texture Declaration
            name = reader.read_str();
            textures[name] = new Texture(reader);
            std::fprintf(stderr, "Texture Resource \"%s\"\n", name.c_str());
            break;
        case 0xFFFFFF56:    //Motion Declaration
            break;
        case 0xFFFFFF5C:    //Texture Continuation
            name = reader.read_str();
            if(textures[name] != NULL) {
                Texture *decl = dynamic_cast<Texture *>(textures[name]);
                if(decl != NULL) {
                    decl->load_continuation(reader);
                    std::fprintf(stderr, "Texture Continuation \"%s\"\n", name.c_str());
                }
            }
            break;
        case 0xFFFFFF3B:    //CLOD Base Mesh Continuation
            name = reader.read_str();
            if(models[name] != NULL) {
                CLOD_Mesh *decl = dynamic_cast<CLOD_Mesh *>(models[name]);
                if(decl != NULL) {
                    decl->create_base_mesh(reader);
                    std::fprintf(stderr, "CLOD Base Mesh Continuation \"%s\"\n", name.c_str());
                }
            } else {
                std::fprintf(stderr, "CLOD Base Mesh Continuation \"%s\" is not declared.\n", name.c_str());
            }
            break;
        case 0xFFFFFF3C:    //CLOD Progressive Mesh Continuation
            name = reader.read_str();
            if(models[name] != NULL) {
                CLOD_Mesh *decl = dynamic_cast<CLOD_Mesh *>(models[name]);
                if(decl != NULL) {
                    decl->update_resolution(reader);
                    std::fprintf(stderr, "CLOD Progressive Mesh Continuation \"%s\"\n", name.c_str());
                }
            }
            break;
        case 0xFFFFFF3E:    //Point Set Continuation
            name = reader.read_str();
            if(models[name] != NULL) {
                PointSet *decl = dynamic_cast<PointSet *>(models[name]);
                if(decl != NULL) {
                    decl->update_resolution(reader);
                    std::fprintf(stderr, "Point Set Continuation \"%s\"\n", name.c_str());
                }
            }
            break;
        case 0xFFFFFF3F:    //Line Set Continuation
            name = reader.read_str();
            if(models[name] != NULL) {
                LineSet *decl = dynamic_cast<LineSet *>(models[name]);
                if(decl != NULL) {
                    decl->update_resolution(reader);
                    std::fprintf(stderr, "Line Set Continuation \"%s\"\n", name.c_str());
                }
            }
            break;
        default:
            if(0x00000100 <= reader.get_type() && reader.get_type() <= 0x00FFFFFF) {
                std::fprintf(stderr, "New Object block [%08X] is not supported in the current version.\n", reader.get_type());
            } else {
                std::fprintf(stderr, "Unknown block type: 0x%08X.\n", reader.get_type());
            }
            return;
        }
    }
}

GraphicsContext *FileStructure::create_context() {
    GraphicsContext *context = new GraphicsContext();

    for(std::map<std::string, LitTextureShader *>::iterator i = shaders.begin(); i != shaders.end(); i++) {
        context->add_shader_group(i->first, i->second->create_shader_group(materials[i->second->material_name]));
    }
    for(std::map<std::string, Texture *>::iterator i = textures.begin(); i != textures.end(); i++) {
        context->add_texture(i->first, i->second->load_texture());
    }
    for(std::map<std::string, ModelResource *>::iterator i = models.begin(); i != models.end(); i++) {
        context->add_render_group(i->first, i->second->create_render_group());
    }

    return context;
}

SceneGraph *FileStructure::create_scenegraph(const View *view, int pass_index) {
    ViewResource *rsc = views[view->resource_name];
    std::string& root_node_name = rsc->passes[pass_index].root_node_name;
    U3D_LOG << "Root node = " << root_node_name << std::endl;
    Node *root_node = nodes[root_node_name];
    Matrix4f root_node_transform;
    if(!get_world_transform(&root_node_transform, root_node, nodes[""])) {
        U3D_WARNING << "Root node does not belong to the World." << std::endl;
        return NULL;
    }
    Matrix4f view_transform;
    if(!get_world_transform(&view_transform, view, nodes[""])) {
        U3D_WARNING << "View node does not belong to the World." << std::endl;
        return NULL;
    }
    SceneGraph *scene = new SceneGraph(*view, *rsc, view_transform);
    U3D_LOG << "View transform = " << view_transform << std::endl;
    for(std::map<std::string, Node *>::iterator i = nodes.begin(); i != nodes.end(); i++) {
        Matrix4f world_transform;
        Light *light = dynamic_cast<Light *>(i->second);
        if(light != NULL && !light->resource_name.empty()) {
            if(get_world_transform(&world_transform, i->second, nodes[""])) {
                U3D_LOG << "Light node " << light->resource_name << " found." << std::endl;
                LightResource *light_rsc = lights[light->resource_name];
                scene->register_light(*light_rsc, world_transform);
            }
            continue;
        }
        Model *model = dynamic_cast<Model *>(i->second);
        if(model != NULL && !model->resource_name.empty()) {
            if(get_world_transform(&world_transform, i->second, root_node)) {
                U3D_LOG << "Model node " << model->resource_name << " found." << std::endl;
                ModelResource *model_rsc = models[model->resource_name];
                scene->register_model(*model, *model_rsc, root_node_transform * world_transform);
            }
            continue;
        }
    }
    U3D_LOG << "SceneGraph created." << std::endl;
    return scene;
}

void FileStructure::dump_tree_recursive(FILE *fp, std::map<std::string, std::vector<std::string> >& tree, const std::string& name, int depth)
{
    for(int i = 0; i < depth; i++) {
        fputc(' ', fp);
    }
    Light *light = dynamic_cast<Light *>(nodes[name]);
    if(light != NULL) {
        fprintf(fp, "Light <%s> => <%s>\n", name.c_str(), light->resource_name.c_str());
    } else {
        Model *model = dynamic_cast<Model *>(nodes[name]);
        if(model != NULL) {
            fprintf(fp, "Model <%s> => <%s>\n", name.c_str(), model->resource_name.c_str());
        } else {
            View *view = dynamic_cast<View *>(nodes[name]);
            if(view != NULL) {
                ViewResource *view_rsc = views[view->resource_name];
                ViewResource::Pass& first_pass = view_rsc->passes[0];
                fprintf(fp, "View <%s> => <%s>\n", name.c_str(), first_pass.root_node_name.c_str());
            } else {
                fprintf(fp, "Group <%s>\n", name.c_str());
            }
        }
    }
    for(std::vector<std::string>::const_iterator i = tree[name].begin(); i != tree[name].end(); i++) {
        dump_tree_recursive(fp, tree, *i, depth + 1);
    }
}

void FileStructure::dump_tree(FILE *fp)
{
    std::map<std::string, std::vector<std::string> > tree;
    for(std::map<std::string, Node *>::iterator i = nodes.begin(); i != nodes.end(); i++) {
        for(std::vector<Node::Parent>::iterator j = i->second->parents.begin(); j != i->second->parents.end(); j++) {
            tree[j->name].push_back(i->first);
        }
    }
    dump_tree_recursive(fp, tree, "", 0);
}
}
