#include "viewer.hpp"
#include <GL/glew.h>
#include <math.h>

namespace
{
void load_projection_matrix(GLuint index, GLfloat fovx, GLfloat aspect, GLfloat near, GLfloat far)
{
    GLfloat f = 1 / tanf(fovx);
    GLfloat mat[16] = {
        f, 0, 0, 0,
        0, f / aspect, 0, 0,
        0, 0, (near + far) / (near - far), 2 * far * near / (near - far),
        0, 0, -1, 0
    };
    glUniformMatrix4fv(index, 1, GL_FALSE, mat);
}
}

Viewer::Viewer()
{
    glewInit();

    GLuint vertex_array_object;
    glGenVertexArrays(1, &vertex_array_object);
    glBindVertexArray(vertex_array_object);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

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

