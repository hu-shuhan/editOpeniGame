//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbpC_BeamProfile_h
#define kbpC_BeamProfile_h
///////////////////////////////////////////////////////////////////////////////
// kbpC_BeamProfile
//

// Forward declarations
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;

// System include
#include <float.h>

// Begin local includes
#include <mem_AllocationOperators.h>
#include <typ_typeTag.h>
#include <cow_ArrayCOW.h>
#include <cls_IntCOW.h>
#include <cls_Uid.h>
#include <omn_FixedString.h>


class kbpC_BeamProfile : public mem_AllocationOperators
{
public:
    kbpC_BeamProfile();
    // Methods
    virtual void AssertValid() const = 0;
    virtual int NumTempPoints(bool threeD) const = 0;

    // Default methods
    // command string methods
    static omn_FixedString setValuesCmd();
    static omn_FixedString constructorCmd();
    static omn_FixedString profileRepositoryCmd();
    static omn_FixedString nameCmd();

    virtual kbpC_BeamProfile* Copy() const = 0;


    // ODB database interface
    kbpC_BeamProfile(const cls_ReadVisitor& rv);
    static void CowDtor(cls_IntCOW* cow);
    static void InitializeMetadata(cls_ClassRegistrar& reg);
    virtual void DBWrite(const cls_WriteVisitor& wv) const;

    kbpC_BeamProfile* DeepCopy() const;
    virtual typ_typeTag DynTypeId() const;
    static typ_typeTag TypeId();
    // Data members

private:
    cls_Uid m_ClsUid; // ODB database id

};

COW_ARRAYCOW2_DECL(kbpC_BeamProfile, cow_Virtual);

// Default methods implementation
#endif // kbpC_BeamProfile_h
