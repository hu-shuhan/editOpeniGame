#pragma once

#include "SplineUtils/MultiGeo.h"
#include "iGameDrawObject.h"

IGAME_NAMESPACE_BEGIN

class NurbsGeometry : public DrawObject {
public:
    I_OBJECT(NurbsGeometry);
    static Pointer New() { return new NurbsGeometry; }

public:
    IGenum GetDataObjectType() const override;
    bool IsUseSinglePassWireframeRendering() override;

    void SetPatch(std::vector<SplineUtils::Geometry>& geometrys);
    void SetType(SplineUtils::Type type);
    void SetSamples(size_t number);

    //Get real size of DataObject
    IGsize GetRealMemorySize() override;

    void ConvertToDrawableData() override;

protected:
    NurbsGeometry();
    ~NurbsGeometry() override = default;

    void ConvertToCurveData();
    void ConvertToSurfaceData();
    void ConvertToVolumeData();

    // Compute model bounding box
    void ComputeBoundingBox() override;

    SplineUtils::MultiGeo::Pointer m_Geometry;
    int m_Samples = 0;
};

IGAME_NAMESPACE_END
