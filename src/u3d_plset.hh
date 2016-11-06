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
