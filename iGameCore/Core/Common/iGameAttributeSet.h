#ifndef iGameAttributeSet_h
#define iGameAttributeSet_h

#include "iGameFlatArray.h"
#include "iGameElementArray.h"
#include <variant>

IGAME_NAMESPACE_BEGIN
// Attributes set stores all Attribute array data, like scalar, vector etc
class AttributeSet : public Object {
public:
	I_OBJECT(AttributeSet);
	static Pointer New() { return new AttributeSet; }

	struct Attribute
	{
		ArrayObject::Pointer pointer{};
		IGenum type{ IG_NONE };           // IG_SCALAR, IG_VECTOR, IG_NORMAL, IG_TCOORD, IG_TENSOR
		IGenum attachmentType{ IG_NONE }; // IG_POINT, IG_CELL
		bool isDeleted{ false };

		std::pair<float, float> dataRange{ 0.f, 0.f };

		static Attribute None() {
			Attribute att;
			att.pointer = nullptr;
			att.type = IG_NONE;
			att.attachmentType = IG_NONE;
			att.isDeleted = false;
			return att;
		}

		bool isNone() const {
			return pointer == nullptr || type == IG_NONE || attachmentType == IG_NONE || isDeleted == true;
		}
	};

	// Add a scalar attribute to array back.
	IGsize AddScalar(IGenum attachmentType, ArrayObject::Pointer attr);
	// Add a scalar attribute to array back with range.
	IGsize AddScalar(IGenum attachmentType, ArrayObject::Pointer attr, const std::pair<float, float>& range);

	// Add a vector attribute to array back.
	IGsize AddVector(IGenum attachmentType, ArrayObject::Pointer attr);

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
	IGsize AddAttribute(IGenum type, IGenum attachmentType, ArrayObject::Pointer attr, std::pair<float, float> dataRange = { 0.f, 0.f });

	// Get a attribute by index
	Attribute& GetAttribute(const IGsize index);
	const Attribute& GetAttribute(const IGsize index) const;

	// Get a attribute by index and type
	Attribute& GetAttribute(const IGsize index, IGenum type);
	const Attribute& GetAttribute(const IGsize index, IGenum type) const;

	// Get a attribute by name and type
	Attribute& GetAttribute(const std::string& name, IGenum type);
	const Attribute& GetAttribute(const std::string& name, IGenum type) const;

	// Get pointer of a attribute by type,attachmentType and name.
	ArrayObject* GetArrayPointer(IGenum type, IGenum attachmentType,
		const std::string& name);

	// Delete a attribute by index
	void DeleteAttribute(const IGsize index);
	// Get all attributes
	ElementArray<Attribute>::Pointer GetAllAttributes();
	// Get all point attributes, not thread safe
	ElementArray<Attribute>::Pointer GetAllPointAttributes();

	// Get all cell attributes, not thread safe
	ElementArray<Attribute>::Pointer GetAllCellAttributes();
	ElementArray<Attribute>::Pointer GetAllScarleAttributes() {
		if (!m_tmpBuffer) {
			m_tmpBuffer = ElementArray<AttributeSet::Attribute>::New();
		}
		else {
			m_tmpBuffer->Reset();
		}
		auto Scalars = this->GetAllAttributes();
		int size = Scalars->GetNumberOfElements();
		for (int i = 0; i < size; i++) {
			auto scalarDataArray = Scalars->GetElement(i);
			if (scalarDataArray.type == IG_SCALAR) {
				m_tmpBuffer->AddElement(m_Buffer->GetElement(i));
			}

		}
		return m_tmpBuffer;
	};
	void TransformScalars2VectorArray() {

		auto Scalars = this->GetAllScarleAttributes();
		int size = Scalars->GetNumberOfElements();
			for (int i = 0; i < size; i++) {
				auto scalarDataArray = Scalars->GetElement(i);
				auto name = scalarDataArray.pointer->GetName();
				auto attachmentType = scalarDataArray.attachmentType;
                std::cout << "attachmentType:" << attachmentType<< std::endl;
				bool isvector = false;
				if (name[name.length() - 1] == 'X') {
					isvector = true;
					int j = 1;
					for ( j = 1; j < 3; j++) {
						auto scalarDataArray = Scalars->GetElement(i);
						if (i + j >= size)break;
						auto tmpName = Scalars->GetElement(i + j).pointer->GetName();
						if (tmpName[tmpName.length() - 1] != 'X' + j) {
							isvector = false;
						}
					}
				}

				if (isvector) {
					FloatArray::Pointer Vector = FloatArray::New();
					Vector->SetName(name.substr(0, name.length() - 1));
					Vector->SetDimension(3);
					Vector->Resize(scalarDataArray.pointer->GetNumberOfElements());
					float* vector = Vector->RawPointer();
					igIndex index = 0;
					int j = 0;
					for (j = 0; j < 3; j++) {
						auto scalarData = Scalars->GetElement(i + j).pointer;
						index = j;
						int k=0;
						for (k = 0; k < scalarDataArray.pointer->GetNumberOfElements(); k++) {
							vector[index] = scalarData->GetValue(k);
							index += 3;
						}
						//delete scalarData;
					}
					this->AddVector(attachmentType,Vector);
					i += 2;
				}
			}
	}
	void AllocateSizeWithCopy(AttributeSet* ps, IGsize numCells) {
		auto allarray = ps->GetAllAttributes();
		this->m_Buffer->Resize(allarray->Size());
		for (int i = 0; i < allarray->Size(); i++) {
			auto& array = ps->GetAttribute(i);
			if (array.attachmentType == IG_POINT) {
				this->m_Buffer->AddElement(array);
			}
			else {
				Attribute EmptyArray;
				EmptyArray.type = IG_ARRAY_OBJECT;
				EmptyArray.attachmentType = IG_CELL;
				EmptyArray.isDeleted = false;
				this->m_Buffer->AddElement(EmptyArray);
			}
		}
	}
	IGsize GetRealMemorySize() {
		if (!m_Buffer)return 0;
		IGsize res = 0;
		for (int i = 0; i < m_Buffer->Size(); i++) {
			auto array = m_Buffer->GetElement(i).pointer;
			res += array ? array->GetArrayTypedSize() * array->GetNumberOfValues() : 0;
		}
		return res + sizeof(Attribute) * m_Buffer->GetNumberOfElements();
	}
protected:
	AttributeSet();
	~AttributeSet() override = default;

	ElementArray<Attribute>::Pointer m_Buffer{};
	ElementArray<Attribute>::Pointer m_tmpBuffer{};

	Attribute NONE{ AttributeSet::Attribute::None() };
};
IGAME_NAMESPACE_END
#endif