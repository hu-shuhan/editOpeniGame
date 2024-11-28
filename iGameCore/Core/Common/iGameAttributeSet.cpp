#include "iGameAttributeSet.h"

#include <utility>

IGAME_NAMESPACE_BEGIN

IGsize AttributeSet::AddScalar(IGenum attachmentType, ArrayObject::Pointer attr) {
	return this->AddAttribute(IG_SCALAR, attachmentType, attr);
}
IGsize AttributeSet::AddScalar(IGenum attachmentType, ArrayObject::Pointer attr, DoubleArray::Pointer DataRange) {
    return this->AddAttribute(IG_SCALAR, attachmentType, attr, std::move(DataRange));
}
IGsize AttributeSet::AddVector(IGenum attachmentType, ArrayObject::Pointer attr) {
    return this->AddAttribute(IG_VECTOR, attachmentType, attr);
}
IGsize AttributeSet::AddVector(IGenum attachmentType, ArrayObject::Pointer attr, DoubleArray::Pointer DataRange) {
    return this->AddAttribute(IG_VECTOR, attachmentType, attr, std::move(DataRange));
}


//IGsize AttributeSet::AddScalar(IGenum attachmentType, ArrayObject::Pointer attr, const std::pair<float, float>& range) {
//    if (!attr) { return -1; }
//    m_Buffer->AddElement(Attribute{attr, IG_SCALAR, attachmentType, false, range});
//    return m_Buffer->GetNumberOfElements() - 1;
//}
//IGsize AttributeSet::AddVector(IGenum attachmentType,
//	ArrayObject::Pointer attr, const std::pair<float, float>& range) {
//    if (!attr) { return -1; }
//    return this->AddAttribute(IG_VECTOR, attachmentType, attr, range);
//}



AttributeSet::Attribute& AttributeSet::GetScalar()
{
	return GetScalar(0);
}

const AttributeSet::Attribute& AttributeSet::GetScalar() const
{
	return GetScalar(0);
}

AttributeSet::Attribute& AttributeSet::GetScalar(const IGsize index)
{
	return GetAttribute(index, IG_SCALAR);
}

const AttributeSet::Attribute& AttributeSet::GetScalar(const IGsize index) const
{
	return GetAttribute(index, IG_SCALAR);
}

AttributeSet::Attribute& AttributeSet::GetScalar(const std::string& name)
{
	return GetAttribute(name, IG_SCALAR);
}

const AttributeSet::Attribute& AttributeSet::GetScalar(const std::string& name) const
{
	return GetAttribute(name, IG_SCALAR);
}

AttributeSet::Attribute& AttributeSet::GetVector()
{
	return GetVector(0);
}

const AttributeSet::Attribute& AttributeSet::GetVector() const
{
	return GetVector(0);
}

AttributeSet::Attribute& AttributeSet::GetVector(const IGsize index)
{
	return GetAttribute(index, IG_VECTOR);
}

const AttributeSet::Attribute& AttributeSet::GetVector(const IGsize index) const
{
	return GetAttribute(index, IG_VECTOR);
}

AttributeSet::Attribute& AttributeSet::GetVector(const std::string& name)
{
	return GetAttribute(name, IG_VECTOR);
}

const AttributeSet::Attribute& AttributeSet::GetVector(const std::string& name) const
{
	return GetAttribute(name, IG_VECTOR);
}


//IGsize AttributeSet::AddAttribute(IGenum type, IGenum attachmentType,
//	ArrayObject::Pointer attr, std::pair<float, float> dataRange) {
//	if (!attr) { return -1; }
//	m_Buffer->AddElement(Attribute{ attr, type, attachmentType, false , dataRange});
//	return m_Buffer->GetNumberOfElements() - 1;
//}

IGsize AttributeSet::AddAttribute(IGenum type, IGenum attachmentType, const ArrayObject::Pointer& attr) {
    if (!attr) { return -1; }
    m_Buffer->AddElement(Attribute{ attr, type, attachmentType, false});
    return m_Buffer->GetNumberOfElements() - 1;
}
IGsize AttributeSet::AddAttribute(IGenum type, IGenum attachmentType, const ArrayObject::Pointer &attr,
                                  DoubleArray::Pointer dataRange) {

    if (!attr) { return -1; }
    m_Buffer->AddElement(Attribute{ attr, type, attachmentType, false, dataRange});
    return m_Buffer->GetNumberOfElements() - 1;
}


AttributeSet::Attribute& AttributeSet::GetAttribute(const IGsize index) {
	return m_Buffer->ElementAt(index);
}

const AttributeSet::Attribute&
AttributeSet::GetAttribute(const IGsize index) const {
	return m_Buffer->ElementAt(index);
}

AttributeSet::Attribute& AttributeSet::GetAttribute(const IGsize index, IGenum type)
{
	int count = 0;
	for (int i = 0; i < m_Buffer->GetNumberOfElements(); i++) {
		auto& attrb = m_Buffer->GetElement(i);
		if (!attrb.isDeleted && attrb.pointer && attrb.type == type) {
			if (count == index) {
				return attrb;
			}
			count++;
		}
	}
	return NONE;
}

const AttributeSet::Attribute& AttributeSet::GetAttribute(const IGsize index, IGenum type) const
{
	int count = 0;
	for (int i = 0; i < m_Buffer->GetNumberOfElements(); i++) {
		auto& attrb = m_Buffer->GetElement(i);
		if (!attrb.isDeleted && attrb.pointer && attrb.type == type) {
			if (count == index) {
				return attrb;
			}
			count++;
		}
	}
	return NONE;
}
    AttributeSet::Attribute &AttributeSet::GetAttribute(const std::string &name) {
        for (int i = 0; i < m_Buffer->GetNumberOfElements(); i++) {
            auto& attrb = m_Buffer->GetElement(i);
            if (!attrb.isNone() && attrb.pointer->GetName() == name) {
                return attrb;
            }
        }
        return NONE;
    }

    const AttributeSet::Attribute &AttributeSet::GetAttribute(const std::string &name) const {
        for (int i = 0; i < m_Buffer->GetNumberOfElements(); i++) {
            auto& attrb = m_Buffer->GetElement(i);
            if (!attrb.isNone() && attrb.pointer->GetName() == name) {
                return attrb;
            }
        }
        return NONE;
    }


AttributeSet::Attribute& AttributeSet::GetAttribute(const std::string& name, IGenum type)
{
	for (int i = 0; i < m_Buffer->GetNumberOfElements(); i++) {
		auto& attrb = m_Buffer->GetElement(i);
		if (!attrb.isNone() && attrb.type == type && attrb.pointer->GetName() == name) {
			return attrb;
		}
	}
	return NONE;
}

const AttributeSet::Attribute& AttributeSet::GetAttribute(const std::string& name, IGenum type) const
{
	for (int i = 0; i < m_Buffer->GetNumberOfElements(); i++) {
		auto& attrb = m_Buffer->GetElement(i);
		if (!attrb.isNone() && attrb.type == type && attrb.pointer->GetName() == name) {
			return attrb;
		}
	}
	return NONE;
}


ArrayObject* AttributeSet::GetArrayPointer(IGenum type, IGenum attachmentType,
	const std::string& name) {
	for (int i = 0; i < m_Buffer->GetNumberOfElements(); i++) {
		auto& p = GetAttribute(i);
		if (p.isDeleted) continue;
		if (p.attachmentType == attachmentType &&
			p.pointer->GetName() == name) {
			return p.pointer.get();
		}
	}
	return nullptr;
}

void AttributeSet::DeleteAttribute(const IGsize index) {
	if (index < 0 || index >= m_Buffer->GetNumberOfElements()) { return; }
	auto& p = GetAttribute(index);
	p.isDeleted = true;
	p.pointer = nullptr;
}


ElementArray<AttributeSet::Attribute>::Pointer AttributeSet::GetAllAttributes() {
	return m_Buffer;
}

ElementArray<AttributeSet::Attribute>::Pointer AttributeSet::GetAllPointAttributes() {
	if (!m_PointBuffer) {
		m_PointBuffer = ElementArray<AttributeSet::Attribute>::New();
	}
	else {
		m_PointBuffer->Reset();
	}
	for (int i = 0; i < m_Buffer->GetNumberOfElements(); i++) {
		if (m_Buffer->GetElement(i).attachmentType == IG_POINT) {
			m_PointBuffer->AddElement(m_Buffer->GetElement(i));
		}
	}
	return m_PointBuffer;
}

ElementArray<AttributeSet::Attribute>::Pointer AttributeSet::GetAllCellAttributes() {
	if (!m_CellBuffer) {
		m_CellBuffer = ElementArray<AttributeSet::Attribute>::New();
	}
	else {
		m_CellBuffer->Reset();
	}
	for (int i = 0; i < m_Buffer->GetNumberOfElements(); i++) {
		if (m_Buffer->GetElement(i).attachmentType == IG_CELL) {
			m_CellBuffer->AddElement(m_Buffer->GetElement(i));
		}
	}
	return m_CellBuffer;
}

AttributeSet::AttributeSet() { m_Buffer = ElementArray<Attribute>::New(); }



iGame::DoubleArray::Pointer iGame::AttributeSet::Attribute::GetDataRange() {
    if(dataRange == nullptr){
		if (!this->pointer){return dataRange;}
        dataRange = DoubleArray::New();
        int dim = this->pointer->GetDimension();
        dataRange->SetDimension(2);
        dataRange->Resize(dim + 1);
        for(int i = 0; i < dim + 1; i ++){
//            dataRange->SetElement(i, {FLT_MIN, FLT_MAX});
            dataRange->SetElement(i, {0, 0});
        }
		updateAllDataRange();
    }
    return dataRange;
}

bool iGame::AttributeSet::Attribute::updateAllDataRange() {
    if(dataRange == nullptr) {
        GetDataRange();
        return true;
    }
    int dim = this->pointer->GetDimension();
    double dimensionRanges[128];
    for(int i = 0; i < 2 * (dim + 1); i += 2){
        dimensionRanges[i + 0] = dataRange->GetValue(i + 0) == 0 ? DBL_MAX : dataRange->GetValue(i + 0);
        dimensionRanges[i + 1] = dataRange->GetValue(i + 1) == 0 ? DBL_MIN : dataRange->GetValue(i + 1);
    }

    auto& data = this->pointer;
    for(size_t i = 0; i < this->pointer->GetNumberOfValues(); i += dim){
        /* Calc magnitude dimension.*/
        double magnitude_val = 0.f;
        for(int j = 0; j < dim; j ++){
            double val = data->GetValue(i + j);
            magnitude_val += val * val;
        }
        magnitude_val = std::sqrt(magnitude_val);
        dimensionRanges[0] = std::min(magnitude_val, dimensionRanges[0]);
        dimensionRanges[1] = std::max(magnitude_val, dimensionRanges[1]);

        /* Calc every dimension attribute. */
        for(int j = 0; j < dim; j ++){
            double val = data->GetValue(i + j);
            dimensionRanges[2 + 2 * j + 0] = std::min(dimensionRanges[2 + 2 * j + 0], val);
            dimensionRanges[2 + 2 * j + 1] = std::max(dimensionRanges[2 + 2 * j + 1], val);
        }
    }
    for(int i = 0; i < dim + 1; i ++){
        dataRange->SetElement(i, {dimensionRanges[2 * i], dimensionRanges[2 * i + 1]});
    }
    dataRange->Modified();
    return true;
}
IGAME_NAMESPACE_END