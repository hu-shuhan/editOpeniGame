//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/*   -*- mode: c++ -*-   */
#ifndef kprC_PropagatingContainer_h
#define kprC_PropagatingContainer_h

///////////////////////////////////////////////////////////////////////////////
//
// File Name: kprC_PropagatingContainer.h
//
// Description:  Propagating container for EO template
//

// Includes

// Begin local includes
#include <mem_AllocationOperatorsTiny.h>
#include <kprC_StepNameProcedurePair.h>
#include <mdl_Repository.h>
#include <cls_xcMap.h>
#include <cow_ListString.h>
#include <typ_typeTag.h>

// Forward declarations

class cls_ReadVisitor;
class cls_WriteVisitor;
class cls_ClassRegistrar;

#include <rgnC_RegionList.h>
#include <cls_Uid.h>

// Definition

template <class PropagatingEO>
class kprC_PropagatingContainer: public mdl_Repository
{
    typedef cow_ArrayCOW<PropagatingEO, cow_Virtual<PropagatingEO> > PropagatingEOCOW;
  public:

    kprC_PropagatingContainer();
   ~kprC_PropagatingContainer();

    bool Remove(const cow_String&);
    bool Insert(const cow_String&, const PropagatingEO&);
    bool IsMember(const cow_String&) const;
    bool IsEmpty() const;
    void Clear();
    cow_ListString Keys() const;

    int Size() const;
    int UnsuppressedSize() const;

    // ForcedInsert is Insert without an AssertValid check.  This 
    // functionality exists only to allow step replacement to convert
    // std/xpl contact interactions.  This can be eliminated once 
    // contact interactions are unified
    //
    bool ForcedInsert(const cow_String&, const PropagatingEO&);

    rgnC_RegionList DanglingRegions(const rgnC_RegionList& regions) const;
    rgnC_RegionList DanglingRegions(); 

    // List of EO's removed during step replacement
    cow_ListString  InvalidKeys() { return invalidKeys;}

    PropagatingEO& Get(const cow_String&);
    const PropagatingEO& ConstGet(const cow_String&) const;

    virtual mdl_Extension* Copy() const;
    virtual mdl_Extension* DeepCopy() const;

    // Must write function to fixup container when steps change

    void UpdateSteps(const kprC_StepNameProcedurePairList&,
		     const cow_String&,
		     bool suppress);
    void ClearStep(const cow_String& stepName);
    void ClearSteps();

    // Messaging/database interface
    virtual typ_typeTag     DynTypeId() const;
    static typ_typeTag      TypeId();

    kprC_PropagatingContainer( const cls_ReadVisitor& rv, bool wrapped = false );
    static void* Ctor( cls_ReadVisitor& rv );
    virtual void DBWrite( const cls_WriteVisitor& wv ) const;
    virtual void DBWrite( const cls_WriteVisitor& wv, bool wrapped ) const { DBWrite(wv); }
    static void InitializeMetadata( cls_ClassRegistrar& reg );
    void NewUid();

    // Timestamp
    void UpdateTimeStamp( const cow_String& aa );
    double GetTimeStamp() const { return _timestamp; }
    void ResetDeltaList() { deltaList.Clear(); }
    cow_ListString DeltaList() { return deltaList; } 

  private:

    // Must write functions to fixup container when steps change

    void InsertStep(const kprC_StepNameProcedurePairList&);
    void RemoveStep(const kprC_StepNameProcedurePairList&);
    void RenameStep(const kprC_StepNameProcedurePairList&);
    void ReplaceStep(const kprC_StepNameProcedurePairList&);
    void SuppressResumeStep(const kprC_StepNameProcedurePairList&,
			    const cow_String& suppressResumeStepName,
			    bool suppressStep);


  // ***********************
  // **     WARNING       **
  // ***********************
  //
  // If any member is added to this template class, the
  // member must also be deep-copied in the DeepCopy() method.
  // 
    // Members
    kprC_StepNameProcedurePairList procedures;
    cls_xcMap<cow_String,PropagatingEOCOW,PropagatingEO> map;

    rgnC_RegionList danglingRegions;  //Temp variable to clean up dangling regions
    cls_Uid m_ClsUid;

    double _timestamp; // container timestamp;
    cow_ListString deltaList; // List of objects with stale glyphs
    cow_ListString invalidKeys;
};





//////////////////////////////////////////////////////////////
// Iterator class

template <class PropagatingEO>
class kprC_PropagatingContainerIT : public mem_AllocationOperatorsTiny
{
  public:

    kprC_PropagatingContainerIT(
	const kprC_PropagatingContainer<PropagatingEO>& c): 
	    iter(c.Keys()), map(c) {}

    void First() { iter.First(); }
    void Next()  { iter.Next(); }
    bool IsDone() const { return iter.IsDone(); }
    
    const cow_String& CurrentKey() const 
        { return iter.CurrentItem(); }
    const PropagatingEO& CurrentValue() const 
        { return map.ConstGet(iter.CurrentItem()); }

  private:

    cow_ListStringIT iter;
    const kprC_PropagatingContainer<PropagatingEO> map;

};

// Macros for instantiation of the Repositories

#define KPRC_PROPAGATING_CONTAINER_DECL(TYPE,NAME) \
CLS_xcMAP_ITER_DECL(cow_String,TYPE ## COW,TYPE ,TYPE ## MAP);\
typedef kprC_PropagatingContainer<TYPE> NAME;\
typedef kprC_PropagatingContainerIT<TYPE> NAME ## IT;




#define KPRC_PROPAGATING_CONTAINER_TT(NAME) \
    {\
        typ_Phase0TT,\
        TYP_TIX_mdl_Repository,\
        #NAME\
    },

#endif /* kprC_PropagatingContainer_h */
