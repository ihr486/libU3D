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

class Viewer
{
public:
    Viewer();
    ~Viewer() {}
    void resize(int width, int height);
    void render();
};

static void debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
    U3D_WARNING << message << std::endl;
}

Viewer::Viewer()
{
    glewInit();

    GLuint vertex_array_object;
    glGenVertexArrays(1, &vertex_array_object);
    glBindVertexArray(vertex_array_object);

    /*glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(debug_callback, NULL);*/

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


#ifdef __WIN32__
#include <windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR lpC, int nC)
#else
int main(int argc, char *argv[])
#endif
{
    std::cout << "Universal 3D loader v0.1a" << std::endl;

#ifndef __WIN32__
    if(argc < 2) {
        std::cerr << "Please specify an input file." << std::endl;
        return 1;
    }

    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Failed to initialize SDL2." << std::endl;
        return 1;
    }

    U3D::FileStructure model(argv[1]);

    std::cerr << argv[1] << " successfully parsed." << std::endl;
#else
    U3D::FileStructure model(lpC);

    std::cerr << (char *)lpC << " successfully parsed." << std::endl;
#endif

    FILE *dump = fopen("tree.log", "w");
    model.dump_tree(dump);
    fclose(dump);

    try {
        U3D::View *defaultview = model.get_view("DefaultView");
        if(defaultview != NULL) {
            U3D_LOG << "DefaultView found." << std::endl;
        } else {
            defaultview = model.get_first_view();
            if(defaultview == NULL) {
                throw U3D_ERROR << "No View node was found.";
            }
        }
        U3D::Matrix4f view_matrix;
        if(model.get_world_transform(&view_matrix, defaultview, model.get_node(""))) {
            U3D_LOG << "View matrix = " << std::endl << view_matrix << std::endl;
        }
        if(!view_matrix.is_view()) {
            U3D_WARNING << "View matrix does not seem to be legal." << std::endl;
        }
        U3D::Matrix4f inverse_view = view_matrix.inverse();
        U3D_LOG << "Inverse view matrix = " << std::endl << inverse_view << std::endl;

        U3D::SceneGraph *scenegraph = model.create_scenegraph(defaultview, 0);

        SDL_Window *window = SDL_CreateWindow("Universal 3D testbed", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 480, 360, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        if(!window) {
            throw U3D_ERROR << "Failed to create an SDL2 window.";
        }

        SDL_GLContext context = SDL_GL_CreateContext(window);
        if(context == 0) {
            throw U3D_ERROR << "Failed to create an OpenGL context.";
        }

        SDL_GL_MakeCurrent(window, context);

        {
            Viewer viewer;

            U3D::GraphicsContext *u3d_context = model.create_context();

            float rotx = 0.0f, roty = 0.0f;
            U3D::Matrix4f original_view_matrix = scenegraph->get_view_matrix();

            SDL_Event event;
            do {
                while(SDL_PollEvent(&event)) {
                    if(event.type == SDL_QUIT) {
                        break;
                    }
                    if(event.type == SDL_WINDOWEVENT) {
                        if(event.window.event == SDL_WINDOWEVENT_RESIZED) {
                            viewer.resize(event.window.data1, event.window.data2);
                        }
                    }
                    if(event.type == SDL_MOUSEMOTION) {
                        if(event.motion.state & SDL_BUTTON_LMASK) {
                            rotx += event.motion.xrel * 0.01f;
                            roty += event.motion.yrel * 0.01f;

                            U3D::Matrix4f vr_matrix = original_view_matrix;
                            U3D::Vector3f vr_offset;
                            vr_offset.x = vr_matrix.m[3][0];
                            vr_offset.y = vr_matrix.m[3][1];
                            vr_offset.z = vr_matrix.m[3][2];
                            vr_matrix.m[3][0] = 0;
                            vr_matrix.m[3][1] = 0;
                            vr_matrix.m[3][2] = 0;
                            U3D::Matrix4f vu_matrix = U3D::Matrix4f::create_X_rotation(rotx) * U3D::Matrix4f::create_Y_rotation(roty);
                            vu_matrix.translate(vr_offset);
                            
                            //U3D::Matrix4f view_matrix = original_view_matrix * U3D::Matrix4f::create_Y_rotation(roty);// * U3D::Matrix4f::create_X_rotation(rotx);

                            scenegraph->set_view_matrix(vu_matrix * vr_matrix);
                        }
                    }
                }
                viewer.render();

                scenegraph->render(u3d_context);

                SDL_GL_SwapWindow(window);

            } while(event.type != SDL_QUIT);

            delete u3d_context;
        }

        delete scenegraph;
    } catch(const U3D::Error& err) {
        std::cerr << err.what() << std::endl;
    }
    SDL_Quit();

    return 0;
}
