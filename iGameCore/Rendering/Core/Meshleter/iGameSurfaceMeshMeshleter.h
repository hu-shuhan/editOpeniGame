//
// Created by Sumzeek on 1/4/2025.
//

#pragma once

#include "OpenGL/GLBuffer.h"
#include "OpenGL/GLVertexArray.h"
#include "iGameMeshleter.h"
#include "igm/igm.h"
#include "meshoptimizer.h"

IGAME_NAMESPACE_BEGIN

class SurfaceMeshMeshleter : public Meshleter {
public:
    I_OBJECT(SurfaceMeshMeshleter);
    static Pointer New() { return new SurfaceMeshMeshleter; }

protected:
    SurfaceMeshMeshleter();
    ~SurfaceMeshMeshleter() override;

    void Build() override;
};

IGAME_NAMESPACE_END
