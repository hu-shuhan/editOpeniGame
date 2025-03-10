//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_Sorption_h
#define kmaO_Sorption_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_Sorption
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaC_Sorption;

class kmaO_Sorption: public kmaO_MaterialOption
{
  public:
    kmaO_Sorption( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_Sorption();
    kmaO_Sorption( const odb_SequenceSequenceDouble& absorptionTable,
                   odb_String lawAbsorption = "TABULAR",
                   bool exsorption = false,
                   odb_String lawExsorption = "TABULAR",
                   double scanning = 0,
                   const odb_SequenceSequenceDouble& exsorptionTable = odb_SequenceSequenceDouble() );
    kmaO_Sorption( const kmaO_Sorption& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_Sorption& operator=( const kmaO_Sorption& rhs );

    virtual ~kmaO_Sorption();
    odb_String lawAbsorption() const;
    bool exsorption() const;
    odb_String lawExsorption() const;
    double scanning() const;
    odb_SequenceSequenceDouble absorptionTable() const;
    odb_SequenceSequenceDouble exsorptionTable() const;
    const kmaC_Sorption* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_Sorption_h
