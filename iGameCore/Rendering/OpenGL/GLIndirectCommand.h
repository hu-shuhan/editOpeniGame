//
// Created by Sumzeek on 7/21/2024.
//

#pragma once

#include "GLVendor.h"

IGAME_NAMESPACE_BEGIN

typedef struct DrawArraysIndirectCommand_t {
    GLuint count;
    GLuint primCount;
    GLuint first;
    GLuint baseInstance;
} DrawArraysIndirectCommand;

typedef struct DrawElementsIndirectCommand_t {
    GLuint count;
    GLuint primCount;
    GLuint firstIndex;
    GLuint baseVertex;
    GLuint baseInstance;
} DrawElementsIndirectCommand;

IGAME_NAMESPACE_END
