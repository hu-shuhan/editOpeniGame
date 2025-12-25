//
// Created by Sumzeek on 23/02/2024.
//

#ifndef IGAMEVIEW_LITE_UTILS_H
#define IGAMEVIEW_LITE_UTILS_H

#include "types.h"

inline uint32 cycle3(uint32 i){
    uint32 imod3 = i % 3;
    return i - imod3 + ((1<<imod3) & 3);
}

inline uint32 cycle3(uint32 i,uint32 ofs){
    return i - i % 3 + (i + ofs) % 3;
}

#endif //IGAMEVIEW_LITE_UTILS_H
