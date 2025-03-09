//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kamC_Amplitude_h
#define kamC_Amplitude_h
///////////////////////////////////////////////////////////////////////////////
// kamC_Amplitude
//

// Forward declarations
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;

// System include
#include <float.h>

// Begin local includes
#include <mem_AllocationOperators.h>
#include <omu_PrimEnum.h>
#include <cow_ArrayCOW.h>
#include <cls_IntCOW.h>
#include <cls_Uid.h>
#include <omn_FixedString.h>


class kamC_Amplitude : public mem_AllocationOperators
{
public:
    kamC_Amplitude();
    virtual ~kamC_Amplitude() {};
    // Enumerations
    enum TimeSpanEnm {
        STEP,
        TOTAL
    };
    // Methods
    void AssertValid() const;

    // Default methods
    static inline TimeSpanEnm timeSpanDef();
    // command string methods
    static omn_FixedString timeSpanCmd();
    static omn_FixedString setValuesCmd();
    static omn_FixedString constructorCmd();
    static omn_FixedString amplitudeRepositoryCmd();
    static omn_FixedString nameCmd();

    virtual kamC_Amplitude* Copy() const = 0;


    // ODB database interface
    kamC_Amplitude(const cls_ReadVisitor& rv);
    static void CowDtor(cls_IntCOW* cow);
    static void InitializeMetadata(cls_ClassRegistrar& reg);
    virtual void DBWrite(const cls_WriteVisitor& wv) const;

    kamC_Amplitude* DeepCopy() const;
    virtual typ_typeTag DynTypeId() const;
    static typ_typeTag TypeId();
    // Data members
    TimeSpanEnm timeSpan;

private:
    cls_Uid m_ClsUid; // ODB database id

};

COW_ARRAYCOW2_DECL(kamC_Amplitude, cow_Virtual);

typedef omu_PrimEnum<kamC_Amplitude::TimeSpanEnm,1> kamC_AmplitudeTimeSpanConst;

// Default methods implementation
inline kamC_Amplitude::TimeSpanEnm
kamC_Amplitude::timeSpanDef()
{
    return kamC_Amplitude::STEP;
}

#endif // kamC_Amplitude_h
