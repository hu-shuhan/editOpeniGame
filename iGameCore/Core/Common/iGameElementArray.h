#ifndef iGameElementArray_h
#define iGameElementArray_h

#include "iGameObject.h"
#include "iGameVector.h"

IGAME_NAMESPACE_BEGIN
template<typename TElement>
class ElementArray : public Object, private std::vector<TElement> {
public:
    I_OBJECT(ElementArray);
    static Pointer New() { return new ElementArray; }
    using VectorType = std::vector<TElement>;
    using Iterator = typename VectorType::iterator;
    using ConstIterator = typename VectorType::const_iterator;
    using Reference = typename VectorType::reference;
    using ConstReference = typename VectorType::const_reference;

    // Free all memory and initialize the array
    void Initialize() {
        std::vector<TElement> temp;
        this->VectorType::swap(temp);
    }

    // Reallocate memory, and the old memory is preserved. The array
    // size will not change. '_Newcapacity' is the number of elements.
    void Reserve(const IGsize _Newcapacity) {
        this->VectorType::reserve(_Newcapacity);
    }

    // Reallocate memory, and the old memory is preserved. The array
    // size will change. '_Newsize' is the number of elements.
    void Resize(const IGsize _Newsize) { this->VectorType::resize(_Newsize); }
    void Resize(const IGsize _Newsize, const TElement& _Element) {
        this->VectorType::resize(_Newsize, _Element);
    }

    // Reset the array size, and the old memory will not change.
    void Reset() { this->VectorType::clear(); }

    // Free up extra memory.
    void Squeeze() { this->Resize(Size()); }

    bool ShallowCopy(ElementArray<TElement>::Pointer other) { return false; }
    bool DeepCopy(ElementArray<TElement>::Pointer other) {
        if (other == nullptr) return false;
        this->Reserve(other->Size());
        for (int i = 0; i < other->Size(); i++) {
            this->AddElement(other->GetElement(i));
        }
        //this->Modified();
        return true;
    }

    // Add an element to the end of an array
    void AddElement(TElement&& _Element) {
        this->VectorType::push_back(_Element);
    }
    void AddElement(const TElement& _Element) {
        this->VectorType::push_back(_Element);
    }

    // Return element's reference by index _Pos
    Reference ElementAt(const IGsize _Pos) {
        return this->VectorType::operator[](_Pos);
    }
    ConstReference ElementAt(const IGsize _Pos) const {
        return this->VectorType::operator[](_Pos);
    }

    // Get element by index _Pos
    Reference GetElement(const IGsize _Pos) {
        return this->VectorType::operator[](_Pos);
    }
    ConstReference GetElement(const IGsize _Pos) const {
        return this->VectorType::operator[](_Pos);
    }

    // Set element by index _Pos
    void SetElement(const IGsize _Pos, TElement&& _Element) {
        this->VectorType::operator[](_Pos) = _Element;
    }
    void SetElement(const IGsize _Pos, const TElement& _Element) {
        this->VectorType::operator[](_Pos) = _Element;
    }

    Iterator Remove(ConstIterator _Where) {
        return this->VectorType::erase(_Where);
    }

    Iterator Begin() { return this->VectorType::begin(); }
    ConstIterator Begin() const { return this->VectorType::begin(); }
    Iterator End() { return this->VectorType::end(); }
    ConstIterator End() const { return this->VectorType::end(); }

    IGsize GetNumberOfElements() const { return this->VectorType::size(); }

    IGsize Size() const { return this->VectorType::size(); }

    TElement* RawPointer() { return this->VectorType::data(); }

protected:
    ElementArray() {}
    ~ElementArray() override = default;
};
IGAME_NAMESPACE_END
#endif