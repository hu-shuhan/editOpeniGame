//
// Created by Sumzeek on 23/02/2024.
//

#ifndef IGAMEVIEW_LITE_VEC_H
#define IGAMEVIEW_LITE_VEC_H

#include <cmath>
#include "types.h"

struct vec3 {
    float x,y,z;

    bool operator==(const vec3& b)const {
        return x == b.x && y == b.y && z == b.z;
    }
    float& operator[](uint32 i){
        return ((float*)this)[i];
    }
};

inline vec3 operator+(vec3 a,vec3 b){
    return vec3{a.x+b.x,a.y+b.y,a.z+b.z};
}

inline vec3 operator-(vec3 a,vec3 b){
    return vec3{a.x-b.x,a.y-b.y,a.z-b.z};
}

inline vec3 operator*(vec3 a,float b){
    return vec3{a.x*b,a.y*b,a.z*b};
}

inline vec3& operator+=(vec3& a,vec3 b){
    a.x+=b.x,a.y+=b.y,a.z+=b.z;
    return a;
}

inline vec3 operator-(vec3 a){return {-a.x,-a.y,-a.z};}

inline vec3 cross(vec3 a,vec3 b){
    return vec3{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};
}

inline vec3 normalize(vec3 a){
    float rl=1/sqrtf(a.x*a.x+a.y*a.y+a.z*a.z);
    return a*rl;
}

inline float length(vec3 a){
    return sqrtf(a.x*a.x+a.y*a.y+a.z*a.z);
}

inline float length2(vec3 a){
    return a.x*a.x+a.y*a.y+a.z*a.z;
}

#endif //IGAMEVIEW_LITE_VEC_H
