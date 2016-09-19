#pragma once

class Viewer
{
public:
    Viewer();
    ~Viewer() {}
    void resize(int width, int height);
    void render();
};
