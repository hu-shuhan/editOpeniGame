#ifndef iGameFlatArray_h
#define iGameFlatArray_h

#include "iGameArrayObject.h"

IGAME_NAMESPACE_BEGIN
template<typename TValue>
class FlatArray : public ArrayObject, private std::vector<TValue> {
public:
    I_OBJECT(FlatArray);
    static Pointer New() { return new FlatArray; }
    using VectorType = std::vector<TValue>;
    using Iterator = typename VectorType::iterator;
    using ConstIterator = typename VectorType::const_iterator;
    using Reference = typename VectorType::reference;
    using ConstReference = typename VectorType::const_reference;

    // Free all memory and initialize the array
    void Initialize() override {
        std::vector<TValue> temp;
        this->VectorType::swap(temp);
    }

    // Reallocate memory, and the old data is preserved. The array
    // size will not change. '_NewElementNum' is the number of elements.
    void Reserve(const IGsize _NewElementNum) override {
        this->VectorType::reserve(_NewElementNum * m_Dimension);
    }

    // Reallocate memory, and the old data is preserved. The array
    // size will change. '_Newsize' is the number of elements.
    void Resize(const IGsize _NewElementNum) override {
        this->VectorType::resize(_NewElementNum * m_Dimension);
    }

    // Reset the array size, and the old memory will not change.
    void Reset() override { this->VectorType::clear(); }

    // Reset the array size, and the old memory will not change.
    void Clear() { this->VectorType::clear(); }

    // Free unnecessary memory.
    void Squeeze() override { this->Resize(GetNumberOfElements()); }


    bool ShallowCopy(FlatArray<TValue>::Pointer other) { return false; }
    bool DeepCopy(FlatArray<TValue>::Pointer other) {
        if (other == nullptr) { return false; }
        SetName(other->GetName());

        m_Dimension = other->m_Dimension;
        this->Reserve(other->GetNumberOfElements());
        for (IGsize i = 0; i < other->GetNumberOfValues(); i++) {
            this->AddValue(other->RawPointer()[i]);
        }

        return true;
    }

    // Set the size of the element
    void SetDimension(const int _Newsize) override {
        assert(_Newsize > 0);
        m_Dimension = _Newsize;
    }
    // Get the size of the element
    int GetDimension() override { return m_Dimension; }
    IGsize GetNumberOfValues() const override {
        return this->VectorType::size();
    }
    IGsize GetNumberOfElements() const override {
        return this->GetNumberOfValues() / m_Dimension;
    }
    IGsize GetCapacity() const { return this->VectorType::capacity(); }

    // Add a element to array back. Return the index of element
    template<int dimension_t>
    IGsize AddElement(Vector<TValue, dimension_t>&& _Element) {
        assert(dimension_t >= m_Dimension);
        IGsize index = this->GetNumberOfElements();
        if (index * m_Dimension >= this->GetCapacity()) {
            this->Reserve(2 * index + 1);
        }

        for (int i = 0; i < m_Dimension; ++i) {
            this->VectorType::push_back(_Element[i]);
        }
        return index;
    }
    template<int dimension_t>
    IGsize AddElement(const Vector<TValue, dimension_t>& _Element) {
        assert(dimension_t >= m_Dimension);
        IGsize index = this->GetNumberOfElements();
        if (index * m_Dimension >= this->GetCapacity()) {
            this->Reserve(2 * index + 1);
        }

        for (int i = 0; i < m_Dimension; ++i) {
            this->VectorType::push_back(_Element[i]);
        }
        return index;
    }
    IGsize AddElement(const std::vector<TValue>& _Element) {
        assert(_Element.size() >= m_Dimension);
        IGsize index = this->GetNumberOfElements();
        if (index * m_Dimension >= this->GetCapacity()) {
            this->Reserve(2 * index + 1);
        }

        for (int i = 0; i < m_Dimension; ++i) {
            this->VectorType::push_back(_Element[i]);
        }
        return index;
    }

    IGsize AddElement(int* _Element) override {
        IGsize index = this->GetNumberOfElements();
        if (index * m_Dimension >= this->GetCapacity()) {
            this->Reserve(2 * index + 1);
        }

        for (int i = 0; i < m_Dimension; ++i) {
            this->VectorType::push_back(static_cast<TValue>(_Element[i]));
        }
        return index;
    }
    IGsize AddElement(const int* _Element) override {
        IGsize index = this->GetNumberOfElements();
        if (index * m_Dimension >= this->GetCapacity()) {
            this->Reserve(2 * index + 1);
        }

        for (int i = 0; i < m_Dimension; ++i) {
            this->VectorType::push_back(static_cast<TValue>(_Element[i]));
        }
        return index;
    }
    IGsize AddElement(int64_t * _Element) override {
        IGsize index = this->GetNumberOfElements();
        if (index * m_Dimension >= this->GetCapacity()) {
            this->Reserve(2 * index + 1);
        }

        for (int i = 0; i < m_Dimension; ++i) {
            this->VectorType::push_back(static_cast<TValue>(_Element[i]));
        }
        return index;
    }
    IGsize AddElement(const int64_t* _Element) override {
        IGsize index = this->GetNumberOfElements();
        if (index * m_Dimension >= this->GetCapacity()) {
            this->Reserve(2 * index + 1);
        }

        for (int i = 0; i < m_Dimension; ++i) {
            this->VectorType::push_back(static_cast<TValue>(_Element[i]));
        }
        return index;
    }
    IGsize AddElement(float* _Element) override {
        IGsize index = this->GetNumberOfElements();
        if (index * m_Dimension >= this->GetCapacity()) {
            this->Reserve(2 * index + 1);
        }

        for (int i = 0; i < m_Dimension; ++i) {
            this->VectorType::push_back(static_cast<TValue>(_Element[i]));
        }
        return index;
    }
    IGsize AddElement(const float* _Element) override {
        IGsize index = this->GetNumberOfElements();
        if (index * m_Dimension >= this->GetCapacity()) {
            this->Reserve(2 * index + 1);
        }

        for (int i = 0; i < m_Dimension; ++i) {
            this->VectorType::push_back(static_cast<TValue>(_Element[i]));
        }
        return index;
    }
    IGsize AddElement(double* _Element) override {
        IGsize index = this->GetNumberOfElements();
        if (index * m_Dimension >= this->GetCapacity()) {
            this->Reserve(2 * index + 1);
        }

        for (int i = 0; i < m_Dimension; ++i) {
            this->VectorType::push_back(static_cast<TValue>(_Element[i]));
        }
        return index;
    }
    IGsize AddElement(const double* _Element) override {
        IGsize index = this->GetNumberOfElements();
        if (index * m_Dimension >= this->GetCapacity()) {
            this->Reserve(2 * index + 1);
        }

        for (int i = 0; i < m_Dimension; ++i) {
            this->VectorType::push_back(static_cast<TValue>(_Element[i]));
        }
        return index;
    }

    IGsize AddElement(uint8_t* _Element) override {

        IGsize index = this->GetNumberOfElements();
        if (index * m_Dimension >= this->GetCapacity()) {
            this->Reserve(2 * index + 1);
        }

        for (int i = 0; i < m_Dimension; ++i) {
            this->VectorType::push_back(static_cast<TValue>(_Element[i]));
        }
        return index;
    }
    IGsize AddElement(const uint8_t* _Element) override {
        IGsize index = this->GetNumberOfElements();
        if (index * m_Dimension >= this->GetCapacity()) {
            this->Reserve(2 * index + 1);
        }

        for (int i = 0; i < m_Dimension; ++i) {
            this->VectorType::push_back(static_cast<TValue>(_Element[i]));
        }
        return index;
    }
    //IGsize AddElement(TValue* _Element)
    //{
    //	IGsize index = this->GetNumberOfElements();
    //	if (index * m_Dimension >= this->GetCapacity())
    //	{
    //		this->Reserve(2 * index + 1);
    //	}

    //	for (int i = 0; i < m_Dimension; ++i) {
    //		this->VectorType::push_back(_Element[i]);
    //	}
    //	return index;
    //}
    //IGsize AddElement2(TValue val0, TValue val1) {
    //	TValue value[2]{ val0, val1 };
    //	return this->AddElement(value);
    //}
    //IGsize AddElement3(TValue val0, TValue val1, TValue val2) {
    //	TValue value[3]{ val0, val1, val2 };
    //	return this->AddElement(value);
    //}

    // Get the reference of element by index _Pos, '_Element' is a pointer.
    void ElementAt(const IGsize _Pos, TValue*& _Element) {
        assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
        _Element = this->RawPointer(_Pos);
    }
    void ElementAt(const IGsize _Pos, const TValue*& _Element) const {
        assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
        _Element = this->RawPointer(_Pos);
    }
    void ElementAt(const IGsize _Pos, TValue* _Element) {
        assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
        TValue* data = this->RawPointer(_Pos);
        for (int i = 0; i < m_Dimension; ++i) { _Element[i] = data[i]; }
    }
    void ElementAt(const IGsize _Pos, std::vector<TValue>& _Element) const {
        assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
        _Element.clear();
        TValue* data = this->RawPointer(_Pos);
        for (int i = 0; i < m_Dimension; ++i) { _Element.push_back(data[i]); }
    }

    // Get a element by index _Pos. This function is thread-unsafe.
    const std::vector<TValue>& GetElement(const IGsize _Pos) {
        assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
        m_Element.clear();
        TValue* data = this->RawPointer(_Pos);
        for (int i = 0; i < m_Dimension; ++i) { m_Element.push_back(data[i]); }
        return m_Element;
    }

    // Set a element by index _Pos
    template<int dimension_t>
    void SetElement(const IGsize _Pos, Vector<TValue, dimension_t>&& _Element) {
        assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
        assert(dimension_t >= m_Dimension);
        TValue* data = this->RawPointer(_Pos);
        for (int i = 0; i < m_Dimension; ++i) { data[i] = _Element[i]; }
    }
    template<int dimension_t>
    void SetElement(const IGsize _Pos,
                    const Vector<TValue, dimension_t>& _Element) {
        assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
        assert(dimension_t >= m_Dimension);
        TValue* data = this->RawPointer(_Pos);
        for (int i = 0; i < m_Dimension; ++i) { data[i] = _Element[i]; }
    }
    void SetElement(const IGsize _Pos, const std::vector<TValue>& _Element) {
        assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
        assert(_Element.size() >= m_Dimension);
        TValue* data = this->RawPointer(_Pos);
        for (int i = 0; i < m_Dimension; ++i) { data[i] = _Element[i]; }
    }
    //void SetElement(const IGsize _Pos, TValue* _Element) {
    //	assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
    //	TValue* data = this->RawPointer(_Pos);
    //	for (int i = 0; i < m_Dimension; ++i) {
    //		data[i] = _Element[i];
    //	}
    //}
    //void SetElement(const IGsize _Pos, const TValue* _Element) {
    //	assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
    //	TValue* data = this->RawPointer(_Pos);
    //	for (int i = 0; i < m_Dimension; ++i) {
    //		data[i] = _Element[i];
    //	}
    //}
    void SetElement(const IGsize _Pos, int* _Element) override {
        assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
        TValue* data = this->RawPointer(_Pos);
        for (int i = 0; i < m_Dimension; ++i) {
            data[i] = static_cast<TValue>(_Element[i]);
        }
    }
    void SetElement(const IGsize _Pos, const int* _Element) override {
        assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
        TValue* data = this->RawPointer(_Pos);
        for (int i = 0; i < m_Dimension; ++i) {
            data[i] = static_cast<TValue>(_Element[i]);
        }
    }
    void SetElement(const IGsize _Pos, float* _Element) override {
        assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
        TValue* data = this->RawPointer(_Pos);
        for (int i = 0; i < m_Dimension; ++i) {
            data[i] = static_cast<TValue>(_Element[i]);
        }
    }
    void SetElement(const IGsize _Pos, const float* _Element) override {
        assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
        TValue* data = this->RawPointer(_Pos);
        for (int i = 0; i < m_Dimension; ++i) {
            data[i] = static_cast<TValue>(_Element[i]);
        }
    }
    void SetElement(const IGsize _Pos, double* _Element) override {
        assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
        TValue* data = this->RawPointer(_Pos);
        for (int i = 0; i < m_Dimension; ++i) {
            data[i] = static_cast<TValue>(_Element[i]);
        }
    }
    void SetElement(const IGsize _Pos, const double* _Element) override {
        assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
        TValue* data = this->RawPointer(_Pos);
        for (int i = 0; i < m_Dimension; ++i) {
            data[i] = static_cast<TValue>(_Element[i]);
        }
    }

    // Add a value to array back
    IGsize AddValue(TValue _Value) {
        this->VectorType::push_back(_Value);
        return this->VectorType::size();
    }
    // Get the reference of value by index _Pos
    Reference ValueAt(const IGsize _Pos) {
        return this->VectorType::operator[](_Pos);
    }
    ConstReference ValueAt(const IGsize _Pos) const {
        return this->VectorType::operator[](_Pos);
    }

    // Constructed by std::vector or pointer
    bool SetArray(const std::vector<TValue>& _Buffer, int _Dimension) {
        if (_Dimension < 1) { return false; }
        this->VectorType::swap(_Buffer);
        m_Dimension = _Dimension;
        return true;
    }
    bool SetArray(TValue* _DataBuffer, int _Dimension, const IGsize _Size,
                  const IGsize _Capacity) {
        if (_DataBuffer == nullptr || _Dimension < 1 || _Size < 0 ||
            _Capacity < 0) {
            return false;
        }

        std::vector<TValue> vec;
        vec.reserve(_Capacity);
        vec.assign(_DataBuffer, _DataBuffer + _Size);

        this->VectorType::swap(vec);
        m_Dimension = _Dimension;
        return true;
    }


    double GetValue(const IGsize _Pos) override {
        assert(0 <= _Pos && _Pos < this->GetNumberOfValues());
        return static_cast<double>(this->VectorType::operator[](_Pos));
    }
    double GetElementValue(const IGsize _Pos, const int dimension) override {
        assert(0 <= _Pos && _Pos < this->GetNumberOfElements() &&
               dimension < m_Dimension);
        TValue* data = this->RawPointer(_Pos);
        if (dimension >= 0) {
            return static_cast<double>(data[dimension]);
        } else {
            double res = 0.0;
            double value = 0.0;
            for (int i = 0; i < m_Dimension; ++i) {
                value = static_cast<double>(data[i]);
                res += value * value;
            }
            return std::sqrt(res);
        }
    }
    void SetValue(IGsize _Pos, double _Value) override {
        this->VectorType::operator[](_Pos) = static_cast<TValue>(_Value);
    }
    void GetElement(const IGsize _Pos, int* _Element) override {
        assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
        TValue* data = this->RawPointer(_Pos);
        for (int i = 0; i < m_Dimension; ++i) {
            _Element[i] = static_cast<int>(data[i]);
        }
    }
    void GetElement(const IGsize _Pos, float* _Element) override {
        assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
        TValue* data = this->RawPointer(_Pos);
        for (int i = 0; i < m_Dimension; ++i) {
            _Element[i] = static_cast<float>(data[i]);
        }
    }
    void GetElement(const IGsize _Pos, double* _Element) override {
        assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
        TValue* data = this->RawPointer(_Pos);
        for (int i = 0; i < m_Dimension; ++i) {
            _Element[i] = static_cast<double>(data[i]);
        }
    }
    void GetElement(const IGsize _Pos, std::vector<float>& _Element) override {
        assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
        _Element.clear();
        TValue* data = this->RawPointer(_Pos);
        for (int i = 0; i < m_Dimension; ++i) {
            _Element.push_back(static_cast<float>(data[i]));
        }
    }
    void GetElement(const IGsize _Pos, std::vector<double>& _Element) override {
        assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
        _Element.clear();
        TValue* data = this->RawPointer(_Pos);
        for (int i = 0; i < m_Dimension; ++i) {
            _Element.push_back(static_cast<double>(data[i]));
        }
    }

    // Get the raw pointer. '_Pos' is element index
    TValue* RawPointer(const IGsize _Pos = 0) {
        return this->VectorType::data() + _Pos * m_Dimension;
    }
    const TValue* RawPointer(const IGsize _Pos = 0) const {
        return this->VectorType::data() + _Pos * m_Dimension;
    }
    IGsize GetArrayTypedSize() override { return sizeof(TValue); }
    IGsize GetRealMemorySize() { return this->GetCapacity() * sizeof(TValue); }

protected:
    FlatArray() = default;
    ~FlatArray() override = default;

    int m_Dimension{1}; // The size of element

    // The temporary vector to return element, is thread-unsafe.
    std::vector<TValue> m_Element{};
};

#define iGameNewDataArrayMacro(TypeName, ValueType)                            \
    class TypeName : public FlatArray<ValueType> {                             \
    public:                                                                    \
        I_OBJECT(TypeName);                                                    \
        IGenum GetArrayType() override { return IG_##TypeName; }               \
        static Pointer New() { return new TypeName; }                          \
                                                                               \
    protected:                                                                 \
        TypeName() = default;                                                  \
        ~TypeName() override = default;                                        \
    };

iGameNewDataArrayMacro(FloatArray, float) iGameNewDataArrayMacro(DoubleArray,
                                                                 double)

        iGameNewDataArrayMacro(IntArray,
                               int) iGameNewDataArrayMacro(UnsignedIntArray,
                                                           unsigned int)

                iGameNewDataArrayMacro(CharArray, char) iGameNewDataArrayMacro(
                        UnsignedCharArray, unsigned char)

                        iGameNewDataArrayMacro(ShortArray, short)
                                iGameNewDataArrayMacro(UnsignedShortArray,
                                                       unsigned short)

                                        iGameNewDataArrayMacro(LongLongArray,
                                                               long long)
                                                iGameNewDataArrayMacro(
                                                        UnsignedLongLongArray,
                                                        unsigned long long)

                                                        IGAME_NAMESPACE_END
#endif

        /*
#ifndef iGameFlatArray_h
#define iGameFlatArray_h

#include "iGameArrayObject.h"

IGAME_NAMESPACE_BEGIN
template<typename TValue>
class FlatArray : public ArrayObject, private std::vector<TValue> {
public:
I_OBJECT(FlatArray);
static Pointer New() { return new FlatArray; }
using VectorType = std::vector<TValue>;
using Iterator = typename VectorType::iterator;
using ConstIterator = typename VectorType::const_iterator;
using Reference = typename VectorType::reference;
using ConstReference = typename VectorType::const_reference;

// Free all memory and initialize the array
void Initialize() override {
std::vector<TValue> temp;
this->VectorType::swap(temp);
}

// Reallocate memory, and the old data is preserved. The array
// size will not change. '_NewElementNum' is the number of elements.
void Reserve(const IGsize _NewElementNum) override {
IGsize currentCapacity = this->GetCapacity();
IGsize newCapacity = _NewElementNum * m_Dimension;

if (newCapacity != currentCapacity) {
   this->VectorType::reserve(newCapacity);
   this->Modified();
}
}

// Reallocate memory, and the old data is preserved. The array
// size will change. '_Newsize' is the number of elements.
void Resize(const IGsize _NewElementNum) override {
IGsize currentSize = GetNumberOfElements();
IGsize newSize = _NewElementNum;

if (newSize != currentSize) {
   this->VectorType::resize(_NewElementNum * m_Dimension);
   this->Modified();
}
}

// Reset the array size, and the old memory will not change.
void Reset() override {
this->VectorType::clear();
this->Modified();
}

// Free unnecessary memory.
void Squeeze() override {
IGsize currentCapacity = this->VectorType::capacity();
IGsize newCapacity = GetNumberOfElements() * m_Dimension;

if (newCapacity != currentCapacity) {
   this->Resize(GetNumberOfElements());
   this->Modified();
}
}

bool ShallowCopy(FlatArray<TValue>::Pointer other) { return false; }
bool DeepCopy(FlatArray<TValue>::Pointer other) {
if (other == nullptr) { return false; }

m_Dimension = other->m_Dimension;
this->Reserve(other->GetNumberOfElements());
for (IGsize i = 0; i < other->GetNumberOfValues(); i++) {
   this->AddValue(other->RawPointer()[i]);
}
this->Modified();

return true;
}

// Set the size of the element
void SetDimension(const int _Newsize) override {
assert(_Newsize > 0);

if (_Newsize == m_Dimension) {
   return;
} else {
   m_Dimension = _Newsize;
   this->Modified();
}
}
// Get the size of the element
int GetDimension() override { return m_Dimension; }
IGsize GetNumberOfValues() const override {
return this->VectorType::size();
}
IGsize GetNumberOfElements() const override {
return this->GetNumberOfValues() / m_Dimension;
}
IGsize GetCapacity() const { return this->VectorType::capacity(); }

// Add a element to array back. Return the index of element
template<int dimension_t>
IGsize AddElement(Vector<TValue, dimension_t>&& _Element) {
assert(dimension_t >= m_Dimension);
IGsize index = this->GetNumberOfElements();
if (index * m_Dimension >= this->GetCapacity()) {
   this->Reserve(2 * index + 1);
}

for (int i = 0; i < m_Dimension; ++i) {
   this->VectorType::push_back(_Element[i]);
}
this->Modified();
return index;
}
template<int dimension_t>
IGsize AddElement(const Vector<TValue, dimension_t>& _Element) {
assert(dimension_t >= m_Dimension);
IGsize index = this->GetNumberOfElements();
if (index * m_Dimension >= this->GetCapacity()) {
   this->Reserve(2 * index + 1);
}

for (int i = 0; i < m_Dimension; ++i) {
   this->VectorType::push_back(_Element[i]);
}
this->Modified();
return index;
}
IGsize AddElement(const std::vector<TValue>& _Element) {
assert(_Element.size() >= m_Dimension);
IGsize index = this->GetNumberOfElements();
if (index * m_Dimension >= this->GetCapacity()) {
   this->Reserve(2 * index + 1);
}

for (int i = 0; i < m_Dimension; ++i) {
   this->VectorType::push_back(_Element[i]);
}
this->Modified();
return index;
}

IGsize AddElement(int* _Element) override {
IGsize index = this->GetNumberOfElements();
if (index * m_Dimension >= this->GetCapacity()) {
   this->Reserve(2 * index + 1);
}

for (int i = 0; i < m_Dimension; ++i) {
   this->VectorType::push_back(static_cast<TValue>(_Element[i]));
}
this->Modified();
return index;
}
IGsize AddElement(const int* _Element) override {
IGsize index = this->GetNumberOfElements();
if (index * m_Dimension >= this->GetCapacity()) {
   this->Reserve(2 * index + 1);
}

for (int i = 0; i < m_Dimension; ++i) {
   this->VectorType::push_back(static_cast<TValue>(_Element[i]));
}
this->Modified();
return index;
}
IGsize AddElement(float* _Element) override {
IGsize index = this->GetNumberOfElements();
if (index * m_Dimension >= this->GetCapacity()) {
   this->Reserve(2 * index + 1);
}

for (int i = 0; i < m_Dimension; ++i) {
   this->VectorType::push_back(static_cast<TValue>(_Element[i]));
}
this->Modified();
return index;
}
IGsize AddElement(const float* _Element) override {
IGsize index = this->GetNumberOfElements();
if (index * m_Dimension >= this->GetCapacity()) {
   this->Reserve(2 * index + 1);
}

for (int i = 0; i < m_Dimension; ++i) {
   this->VectorType::push_back(static_cast<TValue>(_Element[i]));
}
this->Modified();
return index;
}
IGsize AddElement(double* _Element) override {
IGsize index = this->GetNumberOfElements();
if (index * m_Dimension >= this->GetCapacity()) {
   this->Reserve(2 * index + 1);
}

for (int i = 0; i < m_Dimension; ++i) {
   this->VectorType::push_back(static_cast<TValue>(_Element[i]));
}
this->Modified();
return index;
}
IGsize AddElement(const double* _Element) override {
IGsize index = this->GetNumberOfElements();
if (index * m_Dimension >= this->GetCapacity()) {
   this->Reserve(2 * index + 1);
}

for (int i = 0; i < m_Dimension; ++i) {
   this->VectorType::push_back(static_cast<TValue>(_Element[i]));
}
this->Modified();
return index;
}
//IGsize AddElement(TValue* _Element)
//{
//	IGsize index = this->GetNumberOfElements();
//	if (index * m_Dimension >= this->GetCapacity())
//	{
//		this->Reserve(2 * index + 1);
//	}

//	for (int i = 0; i < m_Dimension; ++i) {
//		this->VectorType::push_back(_Element[i]);
//	}
//	return index;
//}
//IGsize AddElement2(TValue val0, TValue val1) {
//	TValue value[2]{ val0, val1 };
//	return this->AddElement(value);
//}
//IGsize AddElement3(TValue val0, TValue val1, TValue val2) {
//	TValue value[3]{ val0, val1, val2 };
//	return this->AddElement(value);
//}

// Get the reference of element by index _Pos, '_Element' is a pointer.
void ElementAt(const IGsize _Pos, TValue*& _Element) {
assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
_Element = this->RawPointer(_Pos);
}
void ElementAt(const IGsize _Pos, const TValue*& _Element) const {
assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
_Element = this->RawPointer(_Pos);
}
void ElementAt(const IGsize _Pos, TValue* _Element) {
assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
TValue* data = this->RawPointer(_Pos);
for (int i = 0; i < m_Dimension; ++i) { _Element[i] = data[i]; }
}
void ElementAt(const IGsize _Pos, std::vector<TValue>& _Element) const {
assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
_Element.clear();
TValue* data = this->RawPointer(_Pos);
for (int i = 0; i < m_Dimension; ++i) { _Element.push_back(data[i]); }
}

// Get a element by index _Pos. This function is thread-unsafe.
const std::vector<TValue>& GetElement(const IGsize _Pos) {
assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
m_Element.clear();
TValue* data = this->RawPointer(_Pos);
for (int i = 0; i < m_Dimension; ++i) { m_Element.push_back(data[i]); }
return m_Element;
}

// Set a element by index _Pos
template<int dimension_t>
void SetElement(const IGsize _Pos, Vector<TValue, dimension_t>&& _Element) {
assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
assert(dimension_t >= m_Dimension);
TValue* data = this->RawPointer(_Pos);
for (int i = 0; i < m_Dimension; ++i) { data[i] = _Element[i]; }
this->Modified();
}
template<int dimension_t>
void SetElement(const IGsize _Pos,
           const Vector<TValue, dimension_t>& _Element) {
assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
assert(dimension_t >= m_Dimension);
TValue* data = this->RawPointer(_Pos);
for (int i = 0; i < m_Dimension; ++i) { data[i] = _Element[i]; }
this->Modified();
}
void SetElement(const IGsize _Pos, const std::vector<TValue>& _Element) {
assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
assert(_Element.size() >= m_Dimension);
TValue* data = this->RawPointer(_Pos);
for (int i = 0; i < m_Dimension; ++i) { data[i] = _Element[i]; }
this->Modified();
}
//void SetElement(const IGsize _Pos, TValue* _Element) {
//	assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
//	TValue* data = this->RawPointer(_Pos);
//	for (int i = 0; i < m_Dimension; ++i) {
//		data[i] = _Element[i];
//	}
//}
//void SetElement(const IGsize _Pos, const TValue* _Element) {
//	assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
//	TValue* data = this->RawPointer(_Pos);
//	for (int i = 0; i < m_Dimension; ++i) {
//		data[i] = _Element[i];
//	}
//}
void SetElement(const IGsize _Pos, int* _Element) override {
assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
TValue* data = this->RawPointer(_Pos);
for (int i = 0; i < m_Dimension; ++i) {
   data[i] = static_cast<TValue>(_Element[i]);
}
this->Modified();
}
void SetElement(const IGsize _Pos, const int* _Element) override {
assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
TValue* data = this->RawPointer(_Pos);
for (int i = 0; i < m_Dimension; ++i) {
   data[i] = static_cast<TValue>(_Element[i]);
}
this->Modified();
}
void SetElement(const IGsize _Pos, float* _Element) override {
assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
TValue* data = this->RawPointer(_Pos);
for (int i = 0; i < m_Dimension; ++i) {
   data[i] = static_cast<TValue>(_Element[i]);
}
this->Modified();
}
void SetElement(const IGsize _Pos, const float* _Element) override {
assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
TValue* data = this->RawPointer(_Pos);
for (int i = 0; i < m_Dimension; ++i) {
   data[i] = static_cast<TValue>(_Element[i]);
}
this->Modified();
}
void SetElement(const IGsize _Pos, double* _Element) override {
assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
TValue* data = this->RawPointer(_Pos);
for (int i = 0; i < m_Dimension; ++i) {
   data[i] = static_cast<TValue>(_Element[i]);
}
this->Modified();
}
void SetElement(const IGsize _Pos, const double* _Element) override {
assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
TValue* data = this->RawPointer(_Pos);
for (int i = 0; i < m_Dimension; ++i) {
   data[i] = static_cast<TValue>(_Element[i]);
}
this->Modified();
}

// Add a value to array back
IGsize AddValue(TValue _Value) {
this->VectorType::push_back(_Value);
this->Modified();
return this->VectorType::size();
}
// Get the reference of value by index _Pos
Reference ValueAt(const IGsize _Pos) {
return this->VectorType::operator[](_Pos);
}
ConstReference ValueAt(const IGsize _Pos) const {
return this->SuperClass::operator[](_Pos);
}

// Constructed by std::vector or pointer
bool SetArray(const std::vector<TValue>& _Buffer, int _Dimension) {
if (_Dimension < 1) { return false; }
this->VectorType::swap(_Buffer);
m_Dimension = _Dimension;
this->Modified();
return true;
}
bool SetArray(TValue* _DataBuffer, int _Dimension, const IGsize _Size,
         const IGsize _Capacity) {
if (_DataBuffer == nullptr || _Dimension < 1 || _Size < 0 ||
   _Capacity < 0) {
   return false;
}

std::vector<TValue> vec;
vec.reserve(_Capacity);
vec.assign(_DataBuffer, _DataBuffer + _Size);

this->VectorType::swap(vec);
m_Dimension = _Dimension;

this->Modified();
return true;
}


double GetValue(const IGsize _Pos) override {
assert(0 <= _Pos && _Pos < this->GetNumberOfValues());
return static_cast<double>(this->VectorType::operator[](_Pos));
}
double GetElementValue(const IGsize _Pos, const int dimension) override {
assert(0 <= _Pos && _Pos < this->GetNumberOfElements() &&
      dimension < m_Dimension);
TValue* data = this->RawPointer(_Pos);
if (dimension >= 0) {
   return static_cast<double>(data[dimension]);
} else {
   double res = 0.0;
   double value = 0.0;
   for (int i = 0; i < m_Dimension; ++i) {
       value = static_cast<double>(data[i]);
       res += value * value;
   }
   return std::sqrt(res);
}
}
void SetValue(IGsize _Pos, double _Value) override {
this->VectorType::operator[](_Pos) = static_cast<TValue>(_Value);
this->Modified();
}
void GetElement(const IGsize _Pos, int* _Element) override {
assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
TValue* data = this->RawPointer(_Pos);
for (int i = 0; i < m_Dimension; ++i) {
   _Element[i] = static_cast<int>(data[i]);
}
}
void GetElement(const IGsize _Pos, float* _Element) override {
assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
TValue* data = this->RawPointer(_Pos);
for (int i = 0; i < m_Dimension; ++i) {
   _Element[i] = static_cast<float>(data[i]);
}
}
void GetElement(const IGsize _Pos, double* _Element) override {
assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
TValue* data = this->RawPointer(_Pos);
for (int i = 0; i < m_Dimension; ++i) {
   _Element[i] = static_cast<double>(data[i]);
}
}
void GetElement(const IGsize _Pos, std::vector<float>& _Element) override {
assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
_Element.clear();
TValue* data = this->RawPointer(_Pos);
for (int i = 0; i < m_Dimension; ++i) {
   _Element.push_back(static_cast<float>(data[i]));
}
}
void GetElement(const IGsize _Pos, std::vector<double>& _Element) override {
assert(0 <= _Pos && _Pos < this->GetNumberOfElements());
_Element.clear();
TValue* data = this->RawPointer(_Pos);
for (int i = 0; i < m_Dimension; ++i) {
   _Element.push_back(static_cast<double>(data[i]));
}
}

// Get the raw pointer. '_Pos' is element index
TValue* RawPointer(const IGsize _Pos = 0) {
return this->VectorType::data() + _Pos * m_Dimension;
}
const TValue* RawPointer(const IGsize _Pos = 0) const {
return this->VectorType::data() + _Pos * m_Dimension;
}
IGsize GetArrayTypedSize() override { return sizeof(TValue); }
IGsize GetRealMemorySize() { return this->GetCapacity() * sizeof(TValue); }

protected:
FlatArray() = default;
~FlatArray() override = default;

int m_Dimension{1}; // The size of element

// The temporary vector to return element, is thread-unsafe.
std::vector<TValue> m_Element{};
};

#define iGameNewDataArrayMacro(TypeName, ValueType)                            \
class TypeName : public FlatArray<ValueType> {                             \
public:                                                                    \
I_OBJECT(TypeName);                                                    \
IGenum GetArrayType() override { return IG_##TypeName; }               \
static Pointer New() { return new TypeName; }                          \
                                                                      \
protected:                                                                 \
TypeName() = default;                                                  \
~TypeName() override = default;                                        \
};

iGameNewDataArrayMacro(FloatArray, float) iGameNewDataArrayMacro(DoubleArray,
                                                        double)

       iGameNewDataArrayMacro(IntArray,
                              int) iGameNewDataArrayMacro(UnsignedIntArray,
                                                          unsigned int)

               iGameNewDataArrayMacro(CharArray, char) iGameNewDataArrayMacro(
                       UnsignedCharArray, unsigned char)

                       iGameNewDataArrayMacro(ShortArray, short)
                               iGameNewDataArrayMacro(UnsignedShortArray,
                                                      unsigned short)

                                       iGameNewDataArrayMacro(LongLongArray,
                                                              long long)
                                               iGameNewDataArrayMacro(
                                                       UnsignedLongLongArray,
                                                       unsigned long long)

                                                       IGAME_NAMESPACE_END
#endif
*/