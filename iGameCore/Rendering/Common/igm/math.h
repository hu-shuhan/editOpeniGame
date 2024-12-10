#ifndef IGM_MATH_H
#define IGM_MATH_H

#include "mat2x2.h"
#include "mat3x3.h"
#include "mat4x4.h"
#include "vec2.h"
#include "vec3.h"
#include "vec4.h"

namespace igm
{
inline double computePointToLineDis(const igm::vec3& linePoint,
                                    const igm::vec3& dir,
                                    const igm::vec3& point) {
    return std::fabs(igm::cross(dir, point - linePoint).length() /
                     dir.length());
}
} // namespace igm


#endif // IGM_IGM_H