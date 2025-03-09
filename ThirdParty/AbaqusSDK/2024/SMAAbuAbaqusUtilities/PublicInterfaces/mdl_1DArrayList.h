//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef mdl_1DArrayList_h
#define mdl_1DArrayList_h
///////////////////////////////////////////////////////////////////////////////
//
// Name: mdl_1DArrayList.h
//
// Description:  This is an array of cows that is persistent on the database.
// 

// Begin local includes
#include <cls_xc1DArrayList.h>
#include <cow_ArrayCOW.h>
#include <cls_Uid.h>

// Forward declarations
class cls_ReadVisitor;
class cls_WriteVisitor;
class cls_ClassRegistrar;

// For instantiation help, see the macros at the end of this file

// Class definition

template <class C, class T>
class mdl_1DArrayList: public cls_xc1DArrayList<C,T>
{
public:

    mdl_1DArrayList() {}
    mdl_1DArrayList(const cls_xc1DArrayList<C,T>& l):
	cls_xc1DArrayList<C,T>(l) {}
    mdl_1DArrayList(const cow_List<C>& l):
	cls_xc1DArrayList<C,T>(l) {}
    mdl_1DArrayList(int sz): cls_xc1DArrayList<C,T>(sz) {}

    // Interface consistent with cow_List
    int Size() const { 
	return cls_xc1DArrayList<C,T>::Length(); 
    }
    bool IsEmpty() const {
	return cls_xc1DArrayList<C,T>::IsEmpty();
    }
    void Append(const T& item) { 
	cls_xc1DArrayList<C,T>::Append(item); 
    }
    void Append(const mdl_1DArrayList<C,T>& l) { 
	cls_xc1DArrayList<C,T>::Concatenate(l);
    }
    T& Get(int i) { 
	return cls_xc1DArrayList<C,T>::Get(i).Get(); 
    }
    const T& ConstGet(int i) const { 
	return cls_xc1DArrayList<C,T>::ConstGet(i).ConstGet();
    }
    const T& operator[](int i) const { return ConstGet(i); }

    mdl_1DArrayList<C,T> Slice(int start, int last) const {
	return static_cast<cow_List<C> >(cls_xc1DArrayList<C,T>::Slice(start, last));
    }
    bool Remove(int i) { 
	cls_xc1DArrayList<C,T>::RemoveIndex(i);
	return true;
    }
    void ClearAll() { 
	cls_xc1DArrayList<C,T>::Clear(); 
    }
    
    // Messaging/database interface
    mdl_1DArrayList(const cls_ReadVisitor& rv):
	cls_xc1DArrayList<C,T>(rv) {}
    virtual void DBWrite(const cls_WriteVisitor& wv) const {
	cls_xc1DArrayList<C,T>::DBWrite(wv);
    }
};


///////////
//
// Public Instantiation macros.
//

#define MDL_1DARRAYLIST_DECL(TYPE,NAME) \
CLS_xc1DARRAYLIST_DECL(TYPE ## COW, TYPE, NAME ## IMPL); \
typedef mdl_1DArrayList<TYPE ## COW, TYPE> NAME;

#define MDL_1DARRAYLIST_DECL_ADD(TYPE,NAME) \
typedef mdl_1DArrayList<TYPE ## COW, TYPE> NAME;

//
// This macro goes in the .C file.
//
// Remember to include mdl_1DArrayList.T!
//
#define MDL_1DARRAYLIST_IMPL(TYPE,NAME)          \
CLS_xc1DARRAYLIST_IMPL(TYPE ## COW, TYPE, NAME ## IMPL); \
template class mdl_1DArrayList<TYPE ## COW, TYPE>;

#define MDL_1DARRAYLIST_IMPL_ADD(TYPE,NAME)          \
template class mdl_1DArrayList<TYPE ## COW, TYPE>;

#endif  // #ifdef mdl_1DArrayList_h
