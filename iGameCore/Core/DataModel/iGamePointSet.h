#ifndef iGamePointSet_h
#define iGamePointSet_h

#include "iGameDrawObject.h"
#include "iGameMarker.h"
#include "iGamePoints.h"
#include "iGameSelection.h"

IGAME_NAMESPACE_BEGIN
class Model;
class PointSet : public DrawObject {
public:
    I_OBJECT(PointSet);
    static Pointer New() { return new PointSet; }
    IGenum GetDataObjectType() const { return IG_POINT_SET; }
    // Set/Get point array
    void SetPoints(Points::Pointer points);
    Points::Pointer GetPoints();

    // Get the number of points
    IGsize GetNumberOfPoints();

    // Get/Set a point by index ptId
    const Point& GetPoint(const IGsize ptId) const;
    void SetPoint(const IGsize ptId, const Point& p);

    // Add a point to back
    virtual IGsize AddPoint(const Point& p);

    // Request data edit state, only in this state,
    // can perform the adding and delete operation.
    // Adding operations also can be done via GetPoints().
    virtual void RequestEditStatus();

    // Detele point by index ptId
    virtual void DeletePoint(const IGsize ptId);

    // Whether point is deleted or not
    bool IsPointDeleted(const IGsize ptId);

    // Garbage collection to free memory that has been removed
    virtual void GarbageCollection();

    // Whether is in data edit state or not
    bool InEditStatus();

    // Enable data edit state
    void MakeEditStatusOn();
    // Unable data edit state
    void MakeEditStatusOff();
    //Get real size of DataObject
    IGsize GetRealMemorySize() override;
    /* Return original mesh with Display mesh's point index mapping. */
    FlatArray<igIndex>::Pointer GetPointMap();

protected:
    PointSet();
    ~PointSet() override = default;

    // Request point data edit state
    void RequestPointStatus();

    // Compute model bounding box
    void ComputeBoundingBox() override;

    Points::Pointer m_Points{};                  // The point array
    DeleteMarker::Pointer m_PointDeleteMarker{}; // The marker for delete point
    bool m_InEditStatus{false};
    FlatArray<igIndex>::Pointer m_PointMap{nullptr};

public:
    void ConvertToDrawableData() override;

    /* Range's first means minRange, second means maxRange. */
    virtual void SetAttributeWithPointData(ArrayObject::Pointer attr, DoubleArray::Pointer attrRange,
                                           igIndex dimension = -1);

    virtual void SetAttributeWithCellData(ArrayObject::Pointer attr, DoubleArray::Pointer attrRange, igIndex i = -1);

    /* Selection */
public:
    /**
     * @brief 获取模型的选择对象。
     * @return Selection 对象的指针。
     */
    SmartPointer<Selection> GetSelection(Model* model = nullptr);

protected:
    SmartPointer<Selection> m_Selection;
};

IGAME_NAMESPACE_END
#endif
