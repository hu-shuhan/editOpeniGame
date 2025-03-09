//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_PorousBulkModuli_h
#define kmaO_PorousBulkModuli_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_PorousBulkModuli
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_PorousBulkModuli;

class kmaO_PorousBulkModuli: public kmaO_MaterialOption
{
  public:
    kmaO_PorousBulkModuli( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_PorousBulkModuli();
    kmaO_PorousBulkModuli( const odb_SequenceSequenceDouble& table,
                           bool temperatureDependency = false );
    kmaO_PorousBulkModuli( const kmaO_PorousBulkModuli& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_PorousBulkModuli& operator=( const kmaO_PorousBulkModuli& rhs );

    virtual ~kmaO_PorousBulkModuli();
    bool temperatureDependency() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_PorousBulkModuli* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_PorousBulkModuli_h
