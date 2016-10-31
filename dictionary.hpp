#pragma once

namespace U3D
{

class Dictionary
{
    std::vector<RenderGroup *> models;
    std::vector<ShaderGroup *> shaders;
    std::vector<GLuint> textures;
public:
    Dictionary() {
    }
    ~Dictionary() {
        for(std::vector<RenderGroup *>::iterator i = models.begin(); i != models.end(); i++) delete *i;
        for(std::vector<ShaderGroup *>::iterator i = shaders.begin(); i != shaders.end(); i++) delete *i;
        for(std::vector<GLuint>::iterator i = textures.begin(); i != textures.end(); i++) {
            GLuint texture = *i;
            glDeleteTextures(1, &texture);
        }
    }
};

}
