//
// Created by Sumzeek on 23/02/2024.
//

#ifndef IGAMEVIEW_LITE_BOUNDS_H
#define IGAMEVIEW_LITE_BOUNDS_H

#include "utils/vec.h"

struct Bounds{
    vec3 pmin,pmax;
    Bounds(){pmin={1e9,1e9,1e9},pmax={-1e9,-1e9,-1e9};}
    Bounds(vec3 p){pmin=p,pmax=p;}
    Bounds operator+(Bounds b);
    Bounds operator+(vec3 b);
};

struct Sphere{
    vec3 center;
    float radius;

    Sphere operator+(Sphere b);
    static Sphere from_points(vec3* pos,uint32 size);
    static Sphere from_spheres(Sphere* spheres,uint32 size);
};

#endif //IGAMEVIEW_LITE_BOUNDS_H
