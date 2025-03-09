//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2015
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbeO_ConnectorPotential_h
#define kbeO_ConnectorPotential_h
///////////////////////////////////////////////////////////////////////////////
// kbeO_ConnectorPotential
//

// Begin local includes
#include <cow_ArrayCOW.h>
#include <odb_Types.h>
#include <mem_AllocationOperators.h>
#include <kbeC_ConnectorPotentialShortcut.h>
#include <odb_String.h>
#include <kbeO_SequenceCDCTerm.h>
// Forward declarations
class kbeO_DerivedComponent;
class kbeC_ConnectorPotential;
typedef cow_CountedHld<kbeC_ConnectorPotential> kbeC_ConnectorPotentialHld;

class kbeO_ConnectorPotential: public mem_AllocationOperators
{
  public:
    kbeO_ConnectorPotential( const kbeC_ConnectorPotentialShortcut& shortcut );
    kbeO_ConnectorPotential( odb_String componentStyle = "COMPONENT_NUMBER",
                             int componentNumber = 0,
                             odb_String sign = "POSITIVE",
                             double scaleFactor = 1.0,
                             double positiveExponent = 2.0,
                             double shiftFactor = 0.0,
                             odb_String hFunction = "ABS" );
    kbeO_ConnectorPotential( const kbeO_ConnectorPotential& copy );
    virtual kbeO_ConnectorPotential* Copy() const;
    kbeO_ConnectorPotential& operator=( const kbeO_ConnectorPotential& rhs );

    virtual ~kbeO_ConnectorPotential();
    odb_String componentStyle() const;
    int componentNumber() const;
    odb_String sign() const;
    double scaleFactor() const;
    double positiveExponent() const;
    double shiftFactor() const;
    odb_String hFunction() const;
    kbeO_DerivedComponent derivedComponent() const;
    kbeO_DerivedComponent DerivedComponent( const kbeO_SequenceCDCTerm& cdcTerms = kbeO_SequenceCDCTerm() );
    void SetPointer( kbeC_ConnectorPotential* ptr) { pointer = ptr; }
    const kbeC_ConnectorPotential* GetPointer() const;
    kbeC_ConnectorPotentialShortcut Shortcut() const { return shortcut; } 

    bool hasValue() const { return shortcut.HasValue(); }

    static unsigned int typeIdentifier();
    unsigned int subTypeIdentifier() const;

protected:
    kbeC_ConnectorPotentialShortcut shortcut;
    kbeC_ConnectorPotentialHld pointer;
};

COW_ARRAYCOW2_DECL(kbeO_ConnectorPotential, cow_Virtual);

#endif // kbeO_ConnectorPotential_h
