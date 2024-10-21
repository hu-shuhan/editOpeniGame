#include "iGameAttributeSet.h"

IGAME_NAMESPACE_BEGIN

IGsize AttributeSet::AddScalar(IGenum attachmentType,
	ArrayObject::Pointer attr) {
	return this->AddAttribute(IG_SCALAR, attachmentType, attr);
}

IGsize AttributeSet::AddScalar(IGenum attachmentType, ArrayObject::Pointer attr, const std::pair<float, float>& range) {
	if (!attr) { return -1; }
	m_Buffer->AddElement(Attribute{ attr, IG_SCALAR, attachmentType, false, range });
	return m_Buffer->GetNumberOfElements() - 1;
}


IGsize AttributeSet::AddVector(IGenum attachmentType,
	ArrayObject::Pointer attr, const std::pair<float, float>& range) {
    if (!attr) { return -1; }
	return this->AddAttribute(IG_VECTOR, attachmentType, attr, range);
}

IGsize AttributeSet::AddVector(IGenum attachmentType, ArrayObject::Pointer attr) {
    if (!attr) { return -1; }
    return this->AddAttribute(IG_VECTOR, attachmentType, attr);
}

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


IGsize AttributeSet::AddAttribute(IGenum type, IGenum attachmentType,
	ArrayObject::Pointer attr, std::pair<float, float> dataRange) {
	if (!attr) { return -1; }
	m_Buffer->AddElement(Attribute{ attr, type, attachmentType, false , dataRange});
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
	if (!m_tmpBuffer) {
		m_tmpBuffer = ElementArray<AttributeSet::Attribute>::New();
	}
	else {
		m_tmpBuffer->Reset();
	}
	for (int i = 0; i < m_Buffer->GetNumberOfElements(); i++) {
		if (m_Buffer->GetElement(i).attachmentType == IG_POINT) {
			m_tmpBuffer->AddElement(m_Buffer->GetElement(i));
		}
	}
	return m_tmpBuffer;
}


ElementArray<AttributeSet::Attribute>::Pointer AttributeSet::GetAllCellAttributes() {
	if (!m_tmpBuffer) {
		m_tmpBuffer = ElementArray<AttributeSet::Attribute>::New();
	}
	else {
		m_tmpBuffer->Reset();
	}
	for (int i = 0; i < m_Buffer->GetNumberOfElements(); i++) {
		if (m_Buffer->GetElement(i).attachmentType == IG_CELL) {
			m_tmpBuffer->AddElement(m_Buffer->GetElement(i));
		}
	}
	return m_tmpBuffer;
}

AttributeSet::AttributeSet() { m_Buffer = ElementArray<Attribute>::New(); }


IGAME_NAMESPACE_END