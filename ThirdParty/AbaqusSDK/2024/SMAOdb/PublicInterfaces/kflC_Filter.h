//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kflC_Filter_h
#define kflC_Filter_h
///////////////////////////////////////////////////////////////////////////////
// kflC_Filter
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


class kflC_Filter : public mem_AllocationOperators
{
public:
  kflC_Filter(); /// { }
  virtual ~kflC_Filter(); ///  { }

    // Enumerations

    enum OperationEnm {
        NONE,
        MIN,
        MAX,
        ABS
    };
    enum InvariantEnm {
        NONE_2,
        FIRST_2,
        SECOND_2
    };

    // Methods
    virtual void AssertValid() const = 0;

    // Default methods
    static inline int          orderDef();
    static inline OperationEnm operationDef();
    static inline bool         haltDef();
    static inline double       limitDef();
    static inline InvariantEnm invariantDef();

    // command string methods
    static omn_FixedString cutoffFrequencyCmd();
    static omn_FixedString orderCmd();
    static omn_FixedString operationCmd();
    static omn_FixedString haltCmd();
    static omn_FixedString limitCmd();
    static omn_FixedString invariantCmd();
    static omn_FixedString setValuesCmd();
    static omn_FixedString constructorCmd();
    static omn_FixedString filterRepositoryCmd();
    static omn_FixedString nameCmd();

    virtual kflC_Filter* Copy() const = 0;


    // ODB database interface
    kflC_Filter(const cls_ReadVisitor& rv);

    static void    CowDtor(cls_IntCOW* cow);
    static void    InitializeMetadata(cls_ClassRegistrar& reg);
    virtual void   DBWrite(const cls_WriteVisitor& wv) const;

    kflC_Filter*        DeepCopy() const;
    virtual typ_typeTag DynTypeId() const;
    static typ_typeTag  TypeId();

    // Data members
    double        cutoffFrequency;
    int           order;
    OperationEnm  operation;
    bool          halt;
    double        limit;
    InvariantEnm  invariant;

private:
    cls_Uid m_ClsUid; // ODB database id

};

COW_ARRAYCOW2_DECL(kflC_Filter, cow_Virtual);

typedef omu_PrimEnum<kflC_Filter::OperationEnm,1> kflC_FilterOperationConst;
typedef omu_PrimEnum<kflC_Filter::InvariantEnm,1> kflC_FilterInvariantConst;

// Default methods implementation
inline int
kflC_Filter::orderDef()
{
    return 2;
}

inline kflC_Filter::OperationEnm
kflC_Filter::operationDef()
{
    return kflC_Filter::NONE;
}

inline bool
kflC_Filter::haltDef()
{
    return false;
}

inline double
kflC_Filter::limitDef()
{
    return DBL_MAX;
}

inline kflC_Filter::InvariantEnm
kflC_Filter::invariantDef()
{
    return kflC_Filter::NONE_2;
}

#endif // kflC_Filter_h
