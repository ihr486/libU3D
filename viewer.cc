#include "u3d_internal.hh"

Viewer::Viewer()
{
    glewInit();

    GLuint vertex_array_object;
    glGenVertexArrays(1, &vertex_array_object);
    glBindVertexArray(vertex_array_object);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);

    glClearColor(0, 0, 0, 0);
}

void Viewer::resize(int width, int height)
{
    glViewport(0, 0, width, height);
}

void Viewer::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
