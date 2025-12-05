#ifndef iGameAttributeSet_h
#define iGameAttributeSet_h

#include "iGameElementArray.h"
#include "iGameFlatArray.h"
#include <variant>

IGAME_NAMESPACE_BEGIN

class DataObject;

// Attributes set stores all Attribute array data, like scalar, vector etc
class AttributeSet : public Object {
public:
    I_OBJECT(AttributeSet);
    static Pointer New() { return new AttributeSet; }

    struct Attribute {
        ArrayObject::Pointer pointer{nullptr};
        IGenum type{IG_NONE};           // IG_SCALAR, IG_VECTOR, IG_NORMAL, IG_TCOORD, IG_TENSOR
        IGenum attachmentType{IG_NONE}; // IG_POINT, IG_CELL
        bool isDeleted{false};
        /* dataRange is a 2 dimension(Minimum value, Maximum value) flatArray, Here is the meaning of its elements:
         * Element[0] : magnitude data range, if the data only have one dimension, this value is equal to that dimension.
         * Element[1] : x dimension data range.
         * Element[2] : y dimension data range, if exist.
         * Element[3] : z dimension data range, if exist.
         * ...
         * Element[N] : Nth dimension data range, if exist.
         * */
        DoubleArray::Pointer dataRange{nullptr};

        // Get/Set ...
        ArrayObject::Pointer GetPointer();
        void SetPointer(ArrayObject::Pointer o);
        IGenum GetType();
        void SetType(IGenum o);
        IGenum GetAttachmentType();
        void SetAttachmentType(IGenum o);
        bool IsDeleted();
        void Delete();
        DoubleArray::Pointer GetDataRange();
        /* Apply other data ranges to this Attribute. */
        void SetDataRange(DoubleArray::Pointer range);
        bool UpdateAllDataRange();

        // Return None Attribute
        static Attribute None();

        bool DeepCopy(const Attribute& other);

        bool IsNone() const;
    };

    bool ShallowCopy(Pointer o) { return false; }
    bool DeepCopy(Pointer o) {
        if (o == nullptr) return false;
        m_Buffer = ElementArray<Attribute>::New();
        m_Buffer->Resize(o->GetNumberOfAttributes());
        for (int i = 0; i < o->GetNumberOfAttributes(); i++) {
            m_Buffer->GetElement(i).DeepCopy(o->m_Buffer->GetElement(i));
        }
        return true;
    }

    // Add a scalar attribute to array back.
    IGsize AddScalar(IGenum attachmentType, ArrayObject::Pointer attr);
    IGsize AddScalar(IGenum attachmentType, ArrayObject::Pointer attr, DoubleArray::Pointer DataRange);
    // Add a scalar attribute to array back with range.
    //	IGsize AddScalar(IGenum attachmentType, ArrayObject::Pointer attr, const std::pair<float, float>& range);

    // Add a vector attribute to array back.
    //    IGsize AddVector(IGenum attachmentType, ArrayObject::Pointer attr, const std::pair<float, float>& range);
    IGsize AddVector(IGenum attachmentType, ArrayObject::Pointer attr);
    IGsize AddVector(IGenum attachmentType, ArrayObject::Pointer attr, DoubleArray::Pointer DataRange);

    Attribute& GetScalar();
    const Attribute& GetScalar() const;
    Attribute& GetScalar(const IGsize i);
    const Attribute& GetScalar(const IGsize i) const;
    Attribute& GetScalar(const std::string& name);
    const Attribute& GetScalar(const std::string& name) const;

    Attribute& GetVector();
    const Attribute& GetVector() const;
    Attribute& GetVector(const IGsize i);
    const Attribute& GetVector(const IGsize i) const;
    Attribute& GetVector(const std::string& name);
    const Attribute& GetVector(const std::string& name) const;

    // Add a attribute to array back.
    // @param type: The type of attribute, such as IG_SCALAR, IG_VECTOR, IG_NORMAL, IG_TCOORD, IG_TENSOR
    // @param attachmentType: Which element is attached to, such as IG_POINT, IG_CELL
    // @param attr: The pointer of attribute array
    //	IGsize AddAttribute(IGenum type, IGenum attachmentType, ArrayObject::Pointer attr, std::pair<float, float> dataRange = { 0.f, 0.f });
    IGsize AddAttribute(IGenum type, IGenum attachmentType, const ArrayObject::Pointer& attr,
                        DoubleArray::Pointer dataRange);
    IGsize AddAttribute(IGenum type, IGenum attachmentType, const ArrayObject::Pointer& attr);

    // Get a attribute by index
    Attribute& GetAttribute(const IGsize index);
    const Attribute& GetAttribute(const IGsize index) const;

    // Get a attribute by index and type
    Attribute& GetAttribute(const IGsize index, IGenum type);
    const Attribute& GetAttribute(const IGsize index, IGenum type) const;

    // Get a attribute by name
    Attribute& GetAttribute(const std::string& name);
    const Attribute& GetAttribute(const std::string& name) const;
    int GetAttributeIndex(const std::string& name) const;

    // Get a attribute by name and type
    Attribute& GetAttribute(const std::string& name, IGenum type);
    const Attribute& GetAttribute(const std::string& name, IGenum type) const;

    // Get pointer of a attribute by type,attachmentType and name.
    ArrayObject* GetArrayPointer(IGenum type, IGenum attachmentType, const std::string& name);

    // Delete a attribute by index
    void DeleteAttribute(const IGsize index);
    // Set all attributes
    void SetAllAttributes(ElementArray<Attribute>::Pointer buffer);
    // Get all attributes
    ElementArray<Attribute>::Pointer GetAllAttributes();
    // Get all point attributes, not thread safe
    ElementArray<Attribute>::Pointer GetAllPointAttributes();

    // Get all cell attributes, not thread safe
    ElementArray<Attribute>::Pointer GetAllCellAttributes();
    ElementArray<Attribute>::Pointer GetAllScarleAttributes();
    void TransformScalars2VectorArray();

    size_t GetNumberOfAttributes() const;

    IGsize GetRealMemorySize();

    void ForceReConvertToDrawableData();
protected:
    AttributeSet();
    ~AttributeSet() override = default;

    ElementArray<Attribute>::Pointer m_Buffer{};
    ElementArray<Attribute>::Pointer m_PointBuffer{};
    ElementArray<Attribute>::Pointer m_CellBuffer{};
    ElementArray<Attribute>::Pointer m_tmpBuffer{};

    Attribute NONE{AttributeSet::Attribute::None()};

    friend class DataObject;
    DataObject* m_DataObject{nullptr};
};
IGAME_NAMESPACE_END
#endif