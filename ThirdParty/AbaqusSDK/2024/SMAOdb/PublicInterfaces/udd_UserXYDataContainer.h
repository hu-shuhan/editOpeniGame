//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef udd_UserXYDataContainer_h
#define udd_UserXYDataContainer_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <udd_UserXYData.h>
#include <cls_Map2Obj.h>
#include <cls_Uid.h>
#include <cls_xpListString.h>

CLS_MAP2OBJ_ITER_DECL(cow_String, udd_UserXYData, udd_UserXYDataMap);

#define CLS_ATTR_udd_UserXYDataMap(MEMBER) CLS_ATTR_MAPSTRING2OBJ(MEMBER,udd_UserXYDataMap)
#define CLS_ATTR_udd_UserXYDataMap_I(MEMBER) CLS_ATTR_MAPSTRING2OBJ_I(MEMBER,udd_UserXYDataMap)

// Forward declarations
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;

class udd_UserXYDataContainer  : public mem_AllocationOperators
{
     friend class udd_UserXYDataContainerIT;
public:
     udd_UserXYDataContainer();
     ~udd_UserXYDataContainer();

//     bool Remove( const cow_String& );
     bool InsertAfter( const cow_String&, const cow_String&,
		       const udd_UserXYData&);
     bool IsMember(const cow_String&) const;
     bool IsEmpty() const;
//     void Clear();
     int Size() const;

     udd_UserXYData& Get( const cow_String& );
     const udd_UserXYData& ConstGet( const cow_String& ) const;
     
     void Append( const cow_String&, const udd_UserXYData& );

     cow_ListString Keys() const { return order; }

public: // Database interface
    udd_UserXYDataContainer(const cls_ReadVisitor& rv);
    static void* Ctor(cls_ReadVisitor& rv);
    static void InitializeMetadata(cls_ClassRegistrar& reg);
    virtual void DBWrite(const cls_WriteVisitor& wv) const;

    void Flush() const;
    void Release(const cow_String& key ) const;
    void Release() const;

    virtual typ_typeTag DynTypeId() const;
    static typ_typeTag TypeId();


private:
     cls_xpListString order;
     udd_UserXYDataMap map;

     cls_Uid m_ClsUid;
};


// Iterator
class udd_UserXYDataContainerIT  : public mem_AllocationOperators
{
public:
     udd_UserXYDataContainerIT(const udd_UserXYDataContainer& userXYDatacont);
     
     void First() { iter.First(); }
     void Next() {iter.Next(); }
     bool IsDone() { return iter.IsDone(); }
     cow_String CurrentKey() const { return iter.CurrentItem(); }
     const udd_UserXYData& CurrentValue() const;

private:
     const udd_UserXYDataMap& userXYDatas;
     cow_ListStringIT iter;
};

#endif // #ifndef udd_UserXYDataContainer_h
