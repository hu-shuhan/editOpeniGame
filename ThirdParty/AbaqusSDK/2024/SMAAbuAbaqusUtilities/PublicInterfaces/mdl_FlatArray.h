//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef mdl_FlatArray_h
#define mdl_FlatArray_h

// Begin local includes
#include <mem_AllocationOperators.h>

template <class Type> class mdl_FlatArray  : public mem_AllocationOperators
{
public:
     mdl_FlatArray(int length, int width = 1);
     mdl_FlatArray();

     ~mdl_FlatArray();

     void SetPtr(Type* ptr);
     void SetSize(int size);
     inline void SetWidth(int width);

     inline Type* GetPointer();
     inline const Type* ConstGetPointer() const;
     inline int GetSize() const;
     inline int GetWidth() const;

     inline Type& Get(int rowIndex, int columnIndex);
     inline Type ConstGet(int rowIndex, int columnIndex) const;
     inline Type* Get(int rowIndex);
     inline const Type* ConstGet(int rowIndex) const;

     // Needed to instantiate a cow
     virtual mdl_FlatArray<Type>* Copy() const;

private:
     mdl_FlatArray(const mdl_FlatArray<Type>& copy);
     mdl_FlatArray<Type>& operator=(const mdl_FlatArray<Type>& rhs);

     Type* ptr;
     int size;
     int width;
};

//
// SECTION: Inlines
//

template <class Type>
inline void
mdl_FlatArray<Type>::SetWidth(int wth)
{
     width = wth;
}

template <class Type>
inline Type*
mdl_FlatArray<Type>::GetPointer()
{
     return ptr;
}

template <class Type>
inline const Type*
mdl_FlatArray<Type>::ConstGetPointer() const
{
     return ptr;
}

template <class Type>
inline int
mdl_FlatArray<Type>::GetSize() const
{
     return size;
}

template <class Type>
inline int
mdl_FlatArray<Type>::GetWidth() const
{
     return width;
}

template <class Type>
inline Type&
mdl_FlatArray<Type>::Get(int rowIndex, int columnIndex)
{
     return ptr[rowIndex*width + columnIndex];
}

template <class Type>
inline Type
mdl_FlatArray<Type>::ConstGet(int rowIndex, int columnIndex) const
{
     return ptr[rowIndex*width + columnIndex];
}

template <class Type>
inline Type*
mdl_FlatArray<Type>::Get(int rowIndex)
{
     return ptr + rowIndex*width;
}

template <class Type>
inline const Type*
mdl_FlatArray<Type>::ConstGet(int rowIndex) const
{
     return ptr + rowIndex*width;
}

#endif
