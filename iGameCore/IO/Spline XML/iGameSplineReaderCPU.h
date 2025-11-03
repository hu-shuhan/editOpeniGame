/**
 * @class   iGameSplineReaderCPU
 * @brief   iGameSplineReaderCPU's brief
 */

#pragma once

#include "SplineUtils/MultiGeo.h"
#include "iGameSplineGeometry.h"
#include <XML/iGameXMLFileReader.h>

IGAME_NAMESPACE_BEGIN
class SplineReaderCPU : public iGameXMLFileReader {
public:
    I_OBJECT(SplineReaderCPU)
    static Pointer New() { return new SplineReaderCPU; }

    inline void SetSurfaceRenderForVolume(bool surfaceRenderForVolume) {
        m_SurfaceRenderForVolume = surfaceRenderForVolume;
    }

protected:
    SplineReaderCPU();
    ~SplineReaderCPU() override;

    bool Parsing() override;
    bool CreateDataObject() override;

    SplineUtils::Type m_SplineType;
    std::vector<SplineUtils::Geometry> m_Patchs;
    bool m_SurfaceRenderForVolume = false;
};
IGAME_NAMESPACE_END
