#pragma once

#include <iGameXMLFileReader.h>

#if defined(GPSCUDA_ENABLE)
IGAME_NAMESPACE_BEGIN
class SplineReaderGPU : public iGameXMLFileReader {
public:
    I_OBJECT(SplineReaderGPU)

    static Pointer New() { return new SplineReaderGPU; }

    inline void SetSurfaceRenderForVolume(bool surfaceRenderForVolume) {
        m_SurfaceRenderForVolume = surfaceRenderForVolume;
    }

public:
    bool Parsing() override;

protected:
    SplineReaderGPU();
    ~SplineReaderGPU() override = default;
    bool m_SurfaceRenderForVolume = false;
};
IGAME_NAMESPACE_END
#endif
