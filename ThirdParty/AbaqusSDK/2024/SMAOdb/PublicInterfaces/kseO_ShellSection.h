//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseO_ShellSection_h
#define kseO_ShellSection_h
///////////////////////////////////////////////////////////////////////////////
// kseO_ShellSection
//

// Begin local includes
#include <odb_Types.h>
#include <odb_Section.h>
// Forward declarations
class kseO_TransverseShearShell;
class kseC_ShellSection;

class kseO_ShellSection: public odb_Section
{
  public:
    kseO_ShellSection( const kseC_SectionShortcut& shortcut );
    kseO_ShellSection();
    virtual odb_Section* Copy() const;
    virtual ~kseO_ShellSection();
    kseO_TransverseShearShell transverseShear() const;
    kseO_TransverseShearShell TransverseShear( double k11,
                                               double k22,
                                               double k12 );
    static unsigned int typeIdentifier();
};

#endif // kseO_ShellSection_h
