/**
 * @class    SurfaceMeshMeshleter
 * @brief    SurfaceMeshMeshleter类用于处理和生成基于表面的Meshlet。
 *
 * SurfaceMeshMeshleter继承自Meshleter类，专注于对表面几何体的Meshlet构建和优化。
 * 该类重载了基类的Build方法，用于实现特定的表面Meshlet生成逻辑。
 *
 * @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
 */

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

    /**
     * @brief 构建函数，重载自Meshleter，用于生成表面Meshlet。
     *
     * 该函数实现了基于表面几何体的Meshlet生成逻辑。
     */
    void Build() override;
};

IGAME_NAMESPACE_END
