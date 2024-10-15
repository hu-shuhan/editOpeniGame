#ifndef iGamePoints_h
#define iGamePoints_h

#include "iGameObject.h"
#include "iGameVector.h"
#include "iGameFlatArray.h"

IGAME_NAMESPACE_BEGIN

typedef Vector3f Point;
class Points : public Object {
public:
	I_OBJECT(Points);
	static Pointer New() { return new Points; }

	// Free all memory and initialize the array
	void Initialize();

	// Reallocate memory, and the old data is preserved. The array
	// size will not change. '_NewPointsNum' is the number of points.
	void Reserve(const IGsize _NewPointsNum);

	// Reallocate memory, and the old data is preserved. The array
	// size will change. '_NewPointsNum' is the number of points.
	void Resize(const IGsize _NewPointsNum);

	// Reset the array size, and the old memory will not change.
	void Reset();

	// Free unnecessary memory.
	void Squeeze();

	bool ShallowCopy(Points::Pointer other) { 
		if (other == nullptr) return false;
		m_Buffer = other->m_Buffer;
		return true; 
	}
    bool DeepCopy(Points::Pointer other) {
        if (other == nullptr) return false;
        m_Buffer = FloatArray::New();
        m_Buffer->DeepCopy(other->m_Buffer);
        return true;
    }

	// Equal to function Resize()
	void SetNumberOfPoints(const IGsize _NewPointsNum);

	IGsize GetNumberOfPoints() const noexcept;

	// Set/Get/Add a point
	void GetPoint(const IGsize ptId, Vector3d& p);
	void GetPoint(const IGsize ptId, Vector3f& p);
	Point& GetPoint(const IGsize ptId);
	const Point& GetPoint(const IGsize ptId) const;

	void SetPoint(const IGsize ptId, const Vector3d& p);
	void SetPoint(const IGsize ptId, const Vector3f& p);
	void SetPoint(const IGsize ptId, float x, float y, float z);

	IGsize AddPoint(const Vector3d& p);
	IGsize AddPoint(const Vector3f& p);
	IGsize AddPoint(float x, float y, float z);
	IGsize AddPoint(float p[3]);
	IGsize AddPoint(double p[3]);

	FloatArray::Pointer ConvertToArray();

	float* RawPointer();

	IGsize GetRealMemorySize() {
		if (!m_Buffer)return 0;
		return this->m_Buffer->GetRealMemorySize() + sizeof(m_Name) + (m_Name.capacity() + 1) * sizeof(char);
	}
protected:
	Points();
	~Points() override = default;

	FloatArray::Pointer m_Buffer{};
};

IGAME_NAMESPACE_END
#endif