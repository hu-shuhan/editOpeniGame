//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef omu_xoObjVarray_h
#define omu_xoObjVarray_h
// Begin local includes

#include <omu_ObjVarray.h>
#include <cls_Uid.h>
#include <typ_typeTag.h>

class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;

//
// This class is basically an omu_ObjVarray with persistence.
// Naming convention comes from the cls containers.
//
template <class T>
class omu_xoObjVarray : public omu_ObjVarray<T>
{
public:

    omu_xoObjVarray();
    omu_xoObjVarray(uint initialSize, double gr=2.0);

    // These constructors should be used when writing temporary objects, that have
    // their UIDs stored in the owner class.
    omu_xoObjVarray( const cls_Uid &externalUid, const cls_Uid &externalDataUid );
    omu_xoObjVarray( const cls_Uid &externalUid, const cls_Uid &externalDataUid, uint initialSize, double gr=2.0 );

    ~omu_xoObjVarray();

private:
    // Copy constructor and assignment operator declared private.
    omu_xoObjVarray(const omu_xoObjVarray<T>&);
    omu_xoObjVarray<T>& operator=(const omu_xoObjVarray<T>&);

public:
    // Messaging/database interface

    virtual typ_typeTag DynTypeId() const;
    static typ_typeTag TypeId();

public:
    // Messaging/Database interface
    omu_xoObjVarray( const cls_ReadVisitor &, cls_Uid *externalUid = 0, cls_Uid *externalDataUid = 0 );
    virtual void DBWrite( const cls_WriteVisitor & ) const;
    static void InitializeMetadata( cls_ClassRegistrar & );

private:
    void DBWriteData( const cls_WriteVisitor & ) const;
    void DBReadData( const cls_ReadVisitor & );
    static const char *DBClassName();

    cls_Uid m_ClsUid;
    cls_Uid m_DataClsUid;

    cls_Uid *m_ExternalUid;
    cls_Uid *m_ExternalDataUid;
};

#define OMU_xoOBJVARRAY_DECL(TYPE, NAME)\
    typedef omu_xoObjVarray<TYPE> NAME

#define OMU_xoOBJVARRAY_IMPL(TYPE, NAME)                                \
    template <> const char *omu_xoObjVarray<TYPE>::DBClassName() { return #NAME; } \
    template <> typ_typeTag omu_xoObjVarray<TYPE>::TypeId() { return TYP_TIX_ ## NAME; } \
    CSL_TYPEADAPTER_SPECIALIZE(NAME);                                   \
    template class omu_ObjVarray<TYPE>;                                 \
    template class omu_xoObjVarray<TYPE>;                               \
    template class cls_AbsTypeAdapter<NAME>;                            \
    template class cls_TypeAdapter<NAME>;

#endif  // #ifdef omu_xoObjVarray_h
