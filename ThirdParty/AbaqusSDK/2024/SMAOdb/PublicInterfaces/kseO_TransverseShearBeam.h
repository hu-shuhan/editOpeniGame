//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseO_TransverseShearBeam_h
#define kseO_TransverseShearBeam_h
///////////////////////////////////////////////////////////////////////////////
// kseO_TransverseShearBeam
//

// Begin local includes
#include <odb_Types.h>
#include <mem_AllocationOperators.h>
#include <kseC_TransShearBeamShortcut.h>
#include <odb_Union.h>
// Forward declarations
class kseC_TransverseShearBeam;

class kseO_TransverseShearBeam: public mem_AllocationOperators
{
  public:
    kseO_TransverseShearBeam( const kseC_TransverseShearBeamShortcut& shortcut );
    kseO_TransverseShearBeam();
    kseO_TransverseShearBeam( const kseO_TransverseShearBeam& copy );
    virtual kseO_TransverseShearBeam* Copy() const;
    kseO_TransverseShearBeam& operator=( const kseO_TransverseShearBeam& rhs );

    virtual ~kseO_TransverseShearBeam();
    odb_Union k23() const;
    odb_Union k13() const;
    odb_Union slendernessCompensation() const;
    kseC_TransverseShearBeamShortcut Shortcut() const { return shortcut; } 

    bool hasValue() const { return shortcut.HasValue(); }

    static unsigned int typeIdentifier();
    unsigned int subTypeIdentifier() const;

protected:
    kseC_TransverseShearBeamShortcut shortcut;
};

#endif // kseO_TransverseShearBeam_h
