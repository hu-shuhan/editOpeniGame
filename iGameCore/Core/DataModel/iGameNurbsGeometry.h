#pragma once

#include "NurbsSDK/MultiGeo.h"
#include "iGameDrawObject.h"

IGAME_NAMESPACE_BEGIN

class NurbsGeometry : public DrawObject {
public:
    I_OBJECT(NurbsGeometry);
    static Pointer New() { return new NurbsGeometry; }

public:
    IGenum GetDataObjectType() const { return IG_NURBS_GEOMETRY; }

    bool IsUseSinglePassWireframeRendering() { return false; }

    void SetPatch(std::vector<NurbsSDK::Geometry>& geometrys);
    void SetBoundary(std::vector<std::array<int, 2>> boundary);
    void SetType(NurbsSDK::NurbsType type);

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

    NurbsSDK::MultiGeo::Pointer m_Geometry;

    //void Draw(Scene* scene) override;
};

IGAME_NAMESPACE_END