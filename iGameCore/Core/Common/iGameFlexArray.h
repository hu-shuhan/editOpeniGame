#ifndef iGameFlexArray_h
#define iGameFlexArray_h

#include "iGameObject.h"

IGAME_NAMESPACE_BEGIN
template<class _Ty, int _Init_Capacity = 127>
class FlexArray {
public:
    static_assert(_Init_Capacity > 0 &&
                  "_Init_Capacity need greater than zero!");
    using ValueType = _Ty;
    using reference = _Ty&;
    using const_reference = const _Ty&;

	FlexArray() {
        m_Pointer = m_InitBuffer;
        m_Capacity = _Init_Capacity;
    }
    ~FlexArray() {
        if (m_Pointer != m_InitBuffer) { 
            delete[] m_Pointer; 
        }
    }

    IGsize size() const noexcept { return m_Size; }
    IGsize capacity() const noexcept { return m_Capacity; }
    void reset() { m_Size = 0; }
    void resize(int size) { m_Size = size; }

    void push_back(_Ty&& _Value) 
    {
        if (m_Size == m_Capacity) 
        {
            _Ty* oldPointer = m_Pointer;
            m_Capacity = std::max(m_Capacity + 1,
                                  static_cast<IGsize>(m_Capacity * 1.5));
            m_Pointer = new _Ty[m_Capacity];

            for (IGsize i = 0; i < m_Size; i++) 
            {
                m_Pointer[i] = oldPointer[i];
            }
            if (oldPointer != m_InitBuffer) 
            { 
                delete[] oldPointer; 
            }
        }

        m_Pointer[m_Size++] = _Value;
    }

    void push_back(const _Ty& _Value) {
        if (m_Size == m_Capacity) {
            _Ty* oldPointer = m_Pointer;
            m_Capacity = std::max(m_Capacity + 1,
                                  static_cast<IGsize>(m_Capacity * 1.5));
            m_Pointer = new _Ty[m_Capacity];

            for (IGsize i = 0; i < m_Size; i++) {
                m_Pointer[i] = oldPointer[i];
            }
            if (oldPointer != m_InitBuffer) { delete[] oldPointer; }
        }

        m_Pointer[m_Size++] = _Value;
    }

    reference operator[](const IGsize _Pos) {
        assert(0 <= _Pos && _Pos < m_Size);
        return m_Pointer[_Pos];
    }
    const_reference operator[](const IGsize _Pos) const {
        assert(0 <= _Pos && _Pos < m_Size);
        return m_Pointer[_Pos];
    }

    reference at(const IGsize _Pos) {
        assert(0 <= _Pos && _Pos < m_Size);
        return m_Pointer[_Pos];
    }
    const_reference at(const IGsize _Pos) const {
        assert(0 <= _Pos && _Pos < m_Size);
        return m_Pointer[_Pos];
    }

protected:
    ValueType m_InitBuffer[_Init_Capacity];
    ValueType* m_Pointer{nullptr};
    IGsize m_Size{0};
    IGsize m_Capacity{0};
};
IGAME_NAMESPACE_END
#endif