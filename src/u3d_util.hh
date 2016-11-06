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

class Error : public std::exception
{
    std::string msg;
    std::ostringstream stream;
public:
    Error(const char *filename, int line) {
        stream << filename << ":" << line << ":";
    }
    Error(const Error& old) : msg(old.stream.str()) {
    }
    virtual const char *what() const throw() {
        return msg.c_str();
    }
    template<typename T> Error& operator<<(const T& obj) {
        stream << obj;
        return *this;
    }
    virtual ~Error() throw() {}
};

#define U3D_LOG (std::cout << __FILE__ ":" << __LINE__ << ":")
#define U3D_WARNING (std::cerr << __FILE__ ":" << __LINE__ << ":")
#define U3D_ERROR (U3D::Error(__FILE__, __LINE__))

}
