//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef res_Holder_h
#define res_Holder_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <mdl_FlatArray.h>
#include <cow_COW.h>

template <class Type> class res_Holder  : public mem_AllocationOperators
{
public:
     res_Holder(Type* ptr, int size, int numColumns);
     res_Holder();
     res_Holder(const res_Holder<Type>& c);
     ~res_Holder();

     Type&       Get(int rowIndex, int columnIndex);
     Type*       Get(int rowIndex);

     Type        ConstGet(int rowIndex, int columnIndex) const;
     const Type* ConstGet(int rowIndex) const;

     res_Holder<Type>& operator=(Type* ptr);
     res_Holder<Type>& operator=(const res_Holder<Type>& rhs);

     void SetNumComponents(int numComp);
     void SetSize(int size);

     int  Size() const; 
     int  Width() const;

     Type*       GetPtr() const;
     const Type* ConstGetPtr() const;
     
protected:
     cow_COW<mdl_FlatArray<Type>, cow_Virtual<mdl_FlatArray<Type> > > array;

private:
};

//
// SECTION: MACROS
//
#define res_HOLDER_DECL(TYPE,ALIAS) \
typedef mdl_FlatArray<TYPE> ALIAS ## FLATARRAY; \
COW_COW2_DECL(ALIAS ## FLATARRAY,cow_Virtual); \
typedef res_Holder<TYPE> ALIAS; 



#define res_HOLDER_IMPL(TYPE,ALIAS) \
template class mdl_FlatArray<TYPE>; \
COW_COW2_IMPL(ALIAS ## FLATARRAY, cow_Virtual); \
template class res_Holder<TYPE>; 


#endif
