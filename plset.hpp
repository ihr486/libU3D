#pragma once

#include "bitstream.hpp"
#include "types.hpp"
#include "clod_common.hpp"

namespace U3D
{

class PointSet : private CLOD_Object, public ModelResource
{
    struct Point
    {
        uint32_t shading_id;
        uint32_t position, normal, texcoord[8];
        uint32_t diffuse, specular;
    };
    std::vector<Point> points;
    uint32_t last_diffuse, last_specular, last_texcoord[8];
public:
    PointSet(BitStreamReader& reader);
    void update_resolution(BitStreamReader& reader);
    RenderGroup *create_render_group();
};

class LineSet : private CLOD_Object, public ModelResource
{
    struct Terminal
    {
        uint32_t position, normal, texcoord[8];
        uint32_t diffuse, specular;
    };
    struct Line
    {
        uint32_t shading_id;
        Terminal terminals[2];
        Terminal& get_terminal(uint32_t position) {
            if(terminals[0].position == position) {
                return terminals[0];
            } else {
                return terminals[1];
            }
        }
    };
    std::vector<Line> lines;
    class LineIndexer
    {
        std::vector<std::vector<uint32_t> > line_lists;
    public:
        std::vector<uint32_t> list_lines(uint32_t position) {
            return line_lists[position];
        }
        void add_position() {
            line_lists.push_back(std::vector<uint32_t>());
        }
        void set_line(uint32_t position, uint32_t line) {
            line_lists[position].push_back(line);
        }
    };
    LineIndexer indexer;

    uint32_t last_diffuse, last_specular, last_texcoord[8];
public:
    LineSet(BitStreamReader& reader);
    void update_resolution(BitStreamReader& reader);
    RenderGroup *create_render_group();
};

}
