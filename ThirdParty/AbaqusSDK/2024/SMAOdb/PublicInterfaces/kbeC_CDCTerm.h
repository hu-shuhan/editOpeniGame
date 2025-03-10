//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2015
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbeC_CDCTerm_h
#define kbeC_CDCTerm_h
///////////////////////////////////////////////////////////////////////////////
// kbeC_CDCTerm
//

// Forward declarations
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;

// System include
#include <float.h>

// Begin local includes
#include <kbeC_ConnectorOptions.h>
#include <cls_xp1DArrayListInt.h>
#include <mdl_PropertyTable.h>
#include <omu_PrimEnum.h>
#include <mem_AllocationOperators.h>
#include <typ_typeTag.h>
#include <cow_ArrayCOW.h>
#include <cls_IntCOW.h>
#include <cls_Uid.h>
#include <omn_FixedString.h>


class kbeC_CDCTerm : public mem_AllocationOperators
{
public:
    kbeC_CDCTerm();
    // Enumerations
    enum TermOperatorEnm {
        RSS,
        SUM,
        MACAULEY
    };
    enum TermSignEnm {
        POSITIVE,
        NEGATIVE
    };
    enum IndepCompTypeEnm {
        POSITION,
        MOTION
    };
    // Methods
    virtual void AssertValid() const;
    bool operator==(const kbeC_CDCTerm& ) const;
    bool operator!=(const kbeC_CDCTerm& ) const;
    void SortDataTable();

    // Default methods
    static inline TermOperatorEnm termOperatorDef();
    static inline TermSignEnm termSignDef();
    static inline bool localDependencyDef();
    static inline IndepCompTypeEnm indepCompTypeDef();
    static inline bool tempDependencyDef();
    static inline int fieldDependenciesDef();
    static inline cls_xp1DArrayListInt intrinsicComponentsDef();
    static inline cls_xp1DArrayListInt indepComponentsDef();
    static inline cls_xp2DArrayDouble tableDef();
    // command string methods
    static omn_FixedString termOperatorCmd();
    static omn_FixedString termSignCmd();
    static omn_FixedString localDependencyCmd();
    static omn_FixedString indepCompTypeCmd();
    static omn_FixedString tempDependencyCmd();
    static omn_FixedString fieldDependenciesCmd();
    static omn_FixedString intrinsicComponentsCmd();
    static omn_FixedString indepComponentsCmd();
    static omn_FixedString tableCmd();
    static omn_FixedString optionsCmd();
    static omn_FixedString setValuesCmd();
    static omn_FixedString constructorCmd();

    virtual kbeC_CDCTerm* Copy() const;


    // ODB database interface
    kbeC_CDCTerm(const cls_ReadVisitor& rv);
    static void CowDtor(cls_IntCOW* cow);
    static void InitializeMetadata(cls_ClassRegistrar& reg);
    virtual void DBWrite(const cls_WriteVisitor& wv) const;

    virtual typ_typeTag DynTypeId() const;
    static typ_typeTag TypeId();
    // Data members
    TermOperatorEnm termOperator;
    TermSignEnm termSign;
    bool localDependency;
    IndepCompTypeEnm indepCompType;
    bool tempDependency;
    int fieldDependencies;
    cls_xp1DArrayListInt intrinsicComponents;
    cls_xp1DArrayListInt indepComponents;
    mdl_PropertyTable table;
    kbeC_ConnectorOptionsCOW options;

private:
    cls_Uid m_ClsUid; // ODB database id

};

COW_ARRAYCOW2_DECL(kbeC_CDCTerm, cow_Virtual);

typedef omu_PrimEnum<kbeC_CDCTerm::TermOperatorEnm,1> kbeC_CDCTermTermOperatorConst;
typedef omu_PrimEnum<kbeC_CDCTerm::TermSignEnm,1> kbeC_CDCTermTermSignConst;
typedef omu_PrimEnum<kbeC_CDCTerm::IndepCompTypeEnm,1> kbeC_CDCTermIndepCompTypeConst;

// Default methods implementation
inline kbeC_CDCTerm::TermOperatorEnm
kbeC_CDCTerm::termOperatorDef()
{
    return kbeC_CDCTerm::RSS;
}

inline kbeC_CDCTerm::TermSignEnm
kbeC_CDCTerm::termSignDef()
{
    return kbeC_CDCTerm::POSITIVE;
}

inline bool
kbeC_CDCTerm::localDependencyDef()
{
    return false;
}

inline kbeC_CDCTerm::IndepCompTypeEnm
kbeC_CDCTerm::indepCompTypeDef()
{
    return kbeC_CDCTerm::POSITION;
}

inline bool
kbeC_CDCTerm::tempDependencyDef()
{
    return false;
}

inline int
kbeC_CDCTerm::fieldDependenciesDef()
{
    return 0;
}

inline cls_xp1DArrayListInt
kbeC_CDCTerm::intrinsicComponentsDef()
{
    return cls_xp1DArrayListInt();
}

inline cls_xp1DArrayListInt
kbeC_CDCTerm::indepComponentsDef()
{
    return cls_xp1DArrayListInt();
}

inline cls_xp2DArrayDouble
kbeC_CDCTerm::tableDef()
{
    return cls_xp2DArrayDouble(0,0);
}

#endif // kbeC_CDCTerm_h
