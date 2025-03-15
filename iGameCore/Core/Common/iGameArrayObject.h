#ifndef iGameArrayObject_h
#define iGameArrayObject_h

#include "iGameObject.h"
#include "iGameVector.h"

IGAME_NAMESPACE_BEGIN
class ArrayObject : public Object {
public:
	I_OBJECT(ArrayObject);

	virtual void Initialize() = 0;

    // Reallocate memory, and the old data is preserved. The array
    // size will not change. '_NewElementNum' is the number of elements.
    virtual void Reserve(const IGsize _NewElementNum) = 0;

    // Reallocate memory, and the old data is preserved. The array
    // size will change. '_Newsize' is the number of elements.
    virtual void Resize(const IGsize _NewElementNum) = 0;

    // Reset the array size, and the old memory will not change.
    virtual void Reset() = 0;

    // Free unnecessary memory.
    virtual void Squeeze() = 0;

	// Get the number of elements
	virtual IGsize GetNumberOfElements() const = 0;

	// Get the number of values
	virtual IGsize GetNumberOfValues() const = 0;

	// Set the dimension of the element, such as VECTOR = 3
	virtual void SetDimension(const int _Newsize) = 0;

	// Get the dimension of the element
	virtual int GetDimension() = 0;

	// Get value by index _Pos. Return double type.
	virtual double GetValue(const IGsize _Pos) = 0;
	// Get value by element index _Pos and dimension, dimension=-1 means magnitude. Return double type.
	virtual double GetElementValue(const IGsize _Pos,const int dimension)=0;

	// Set value by index _Pos.
	virtual void SetValue(IGsize _Pos, double value) = 0;

	// Get element by index _Pos.
    virtual void GetElement(const IGsize _Pos, int* _Element) = 0;
	virtual void GetElement(const IGsize _Pos, float* _Element) = 0;
	virtual void GetElement(const IGsize _Pos, double* _Element) = 0;
	virtual void GetElement(const IGsize _Pos, std::vector<float>& _Element) = 0;
	virtual void GetElement(const IGsize _Pos, std::vector<double>& _Element) = 0;

	virtual void SetElement(const IGsize _Pos, int* _Element) = 0;
    virtual void SetElement(const IGsize _Pos, const int* _Element) = 0;
	virtual void SetElement(const IGsize _Pos, float* _Element) = 0;
	virtual void SetElement(const IGsize _Pos, const float* _Element) = 0;
    virtual void SetElement(const IGsize _Pos, double* _Element) = 0;
    virtual void SetElement(const IGsize _Pos, const double* _Element) = 0;

	virtual IGsize AddElement(int* _Element) = 0;
    virtual IGsize AddElement(const int* _Element) = 0;
	virtual IGsize AddElement(float* _Element) = 0;
    virtual IGsize AddElement(const float* _Element) = 0;
	virtual IGsize AddElement(double* _Element) = 0;
    virtual IGsize AddElement(const double* _Element) = 0;
	IGsize AddElement2(double val0, double val1) {
        double value[2]{ val0, val1 };
        return this->AddElement(value);
	}
    IGsize AddElement3(double val0, double val1, double val2) {
        double value[3]{val0, val1, val2};
    	return this->AddElement(value);
	}
    IGsize AddElement4(double val0, double val1, double val2, double val3) {
        double value[4]{val0, val1, val2, val3};
        return this->AddElement(value);
    }

	// Get the type of array.
	virtual IGenum GetArrayType() { return IG_ARRAY_OBJECT; }

	virtual IGsize GetArrayTypedSize() { return 1; };
	// Type down-cast, which is a subclass of ArrayObject
	template<typename _Ty>
	SmartPointer<_Ty> DownCast() {
		return DynamicCast<_Ty>(this);
	}

protected:
	ArrayObject() {}
	~ArrayObject() override = default;
};
IGAME_NAMESPACE_END
#endif