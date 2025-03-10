//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseO_TransverseShearShell_h
#define kseO_TransverseShearShell_h
///////////////////////////////////////////////////////////////////////////////
// kseO_TransverseShearShell
//

// Begin local includes
#include <odb_Types.h>
#include <mem_AllocationOperators.h>
#include <kseC_TransShearShellShortcut.h>
// Forward declarations
class kseC_TransverseShearShell;

class kseO_TransverseShearShell: public mem_AllocationOperators
{
  public:
    kseO_TransverseShearShell( const kseC_TransverseShearShellShortcut& shortcut );
    kseO_TransverseShearShell();
    kseO_TransverseShearShell( const kseO_TransverseShearShell& copy );
    virtual kseO_TransverseShearShell* Copy() const;
    kseO_TransverseShearShell& operator=( const kseO_TransverseShearShell& rhs );

    virtual ~kseO_TransverseShearShell();
    double k11() const;
    double k22() const;
    double k12() const;
    kseC_TransverseShearShellShortcut Shortcut() const { return shortcut; } 

    bool hasValue() const { return shortcut.HasValue(); }

    static unsigned int typeIdentifier();
    unsigned int subTypeIdentifier() const;

protected:
    kseC_TransverseShearShellShortcut shortcut;
};

#endif // kseO_TransverseShearShell_h
