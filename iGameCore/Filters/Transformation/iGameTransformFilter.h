#ifndef iGameTransformFilter_h
#define iGameTransformFilter_h

#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameVolumeMesh.h"
#include "iGameStructuredMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameAttributeSet.h"
#include "iGameFlatArray.h"

#include "iGameCellArray.h"
#include "iGameCellLinks.h"
#include "iGameEdgeTable.h"
#include "iGameFlexArray.h"
#include "iGamePointSet.h"

#include "iGameLine.h"
#include "iGamePolygon.h"
#include "iGameQuad.h"
#include "iGameTriangle.h"

IGAME_NAMESPACE_BEGIN

class TransformFilter : public Filter
{
public:
    I_OBJECT(TransformFilter)

    static Pointer New(){return new TransformFilter;}

    void SetTranslation(float tx,float ty,float tz);

    void SetRotation(float rx,float ry,float rz);

    void SetScale(float sx,float sy,float sz);

    // 旋转 / 缩放的中心点（轴心）。默认 (0,0,0)，即旋转缩放都绕坐标原点，与原有行为完全一致。
    void SetPivot(float px,float py,float pz);

    void BuildMatrix();

    bool Execute() override;


protected:

    TransformFilter();

    ~TransformFilter() override = default;

    float m_Matrix[4][4]{};

    float m_ScaleX{1.0f};
    float m_ScaleY{1.0f};
    float m_ScaleZ{1.0f};

    float m_TranslateX{0.0f};
    float m_TranslateY{0.0f};
    float m_TranslateZ{0.0f};

    float m_RotationX{0.0f};
    float m_RotationY{0.0f};
    float m_RotationZ{0.0f};

    float m_PivotX{0.0f};
    float m_PivotY{0.0f};
    float m_PivotZ{0.0f};
};

IGAME_NAMESPACE_END

#endif
