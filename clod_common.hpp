#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

#include "types.hpp"

namespace U3D
{

class CLOD_Object
{
protected:
    //Maximum mesh description
    uint32_t attributes;
    uint32_t face_count, position_count, normal_count;
    uint32_t diffuse_count, specular_count, texcoord_count;
    struct ShadingDesc {
        uint32_t attributes;
        std::vector<uint32_t> texcoord_dims;
    };
    std::vector<ShadingDesc> shading_descs;
    //CLoD description
    uint32_t min_res, max_res;
    //Quality factors
    uint32_t position_quality, normal_quality, texcoord_quality;
    //Inverse quantization parameters
    float position_iq, normal_iq, texcoord_iq, diffuse_iq, specular_iq;
    //Resource parameters
    float normal_crease, normal_update, normal_tolerance;
    //Skeleton description
    struct Bone {
        std::string name, parent_name;
        uint32_t attributes;
        float length;
        Vector3f displacement;
        Quaternion4f orientation;
        uint32_t link_count;
        float link_length;
        Vector2f start_joint_center, start_joint_scale;
        Vector2f end_joint_center, end_joint_scale;
    };
    std::vector<Bone> skeleton;
public:
    CLOD_Object(BitStreamReader& reader);
};

template<typename T> static inline void insert_unique(std::vector<T>& cont, T val)
{
    typename std::vector<T>::iterator i;
    for(i = cont.begin(); i != cont.end(); i++) {
        if(*i == val) return;
        else if(*i < val) {
            cont.insert(i, val);
            return;
        }
    }
    if(i == cont.end()) {
        cont.push_back(val);
    }
}

template<typename T> static inline void greater_unique_sort(std::vector<T>& cont)
{
    std::sort(cont.begin(), cont.end(), std::greater<T>());
    cont.erase(std::unique(cont.begin(), cont.end()), cont.end());
}

template<typename T> static inline void print_vector(const std::vector<T>& v, const std::string& name)
{
    std::cerr << name << " : ";
    for(typename std::vector<T>::const_iterator i = v.begin(); i != v.end(); i++) {
        std::cerr << *i << " ";
    }
    std::cerr << std::endl;
}

}
