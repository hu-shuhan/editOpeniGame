#ifndef iGameMarker_h
#define iGameMarker_h

#include "iGameObject.h"

IGAME_NAMESPACE_BEGIN
class Marker : public Object {
public:
	I_OBJECT(Marker);

	void Initialize(const IGsize unitCount) {
		this->Reset();
		this->Resize(unitCount);
	}

	void Resize(const IGsize _NewUnitCount) {
		IGsize size = (_NewUnitCount * m_BitsPerUnit + 7) / 8;
		m_Buffer.resize(size, 0);
		m_UnitCount = _NewUnitCount;
	}

	void Reset() {
		m_Buffer.clear();
		m_UnitCount = 0;
	}
	IGsize GetRealMemorySize() {
		return  this->m_Buffer.capacity() * sizeof(uint8_t) + sizeof(m_BitsPerUnit) + sizeof(m_UnitCount);
	}

	bool ShallowCopy(Marker::Pointer other) { return false; }
    bool DeepCopy(Marker::Pointer other) {
        m_BitsPerUnit = other->m_BitsPerUnit;
        m_UnitCount = other->m_UnitCount;
        m_Buffer = other->m_Buffer;
        return true;
    }

protected:
	Marker(int bitsPerUnit) : m_BitsPerUnit(bitsPerUnit) {}
	~Marker() override {}

	int m_BitsPerUnit{ 1 };
	IGsize m_UnitCount{ 0 };
	std::vector<uint8_t> m_Buffer;
};

class DeleteMarker : public Marker {
public:
	I_OBJECT(DeleteMarker);
	static Pointer New() { return new DeleteMarker; }

	void AddTag() {
		if (m_UnitCount % 8 == 0)
		{
			m_Buffer.push_back(0);
		}
		m_UnitCount++;
	}

	void MarkDeleted(const IGsize idx) noexcept {
		m_Buffer[idx / 8] |= (1 << (idx % 8));
		// m_Buffer[idx / 8] &= ~(1 << (idx % 8));
	}

	// cancel delete
	void Undo(const IGsize idx) noexcept {
		m_Buffer[idx / 8] &= ~(1 << (idx % 8));
	}

	bool IsDeleted(const IGsize idx) const noexcept {
		return (m_Buffer[idx / 8] >> (idx % 8)) & 1;
	}



protected:
	DeleteMarker() : Marker(1) {}
	~DeleteMarker() override = default;
};
IGAME_NAMESPACE_END
#endif