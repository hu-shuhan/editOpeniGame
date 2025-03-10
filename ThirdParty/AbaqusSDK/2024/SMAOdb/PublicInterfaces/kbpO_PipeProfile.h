//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbpO_PipeProfile_h
#define kbpO_PipeProfile_h

// Begin local includes
#include <kbpO_Profile.h>
#include <odb_Types.h>
// Forward declarations
class kbpC_PipeProfile;

class kbpO_PipeProfile : public kbpO_Profile
{
public:
    kbpO_PipeProfile( const kbpC_BeamProfileShortcut& shortcut );
    kbpO_PipeProfile();

    kbpO_PipeProfile(double r,
                     double t);

    kbpO_PipeProfile( const kbpO_PipeProfile& copy );

    virtual ~kbpO_PipeProfile();
    double r() const;
    double t() const;
    kbpO_PipeProfile& operator=( const kbpO_PipeProfile& rhs );

    const kbpC_PipeProfile* GetPointer() const;

    static unsigned int typeIdentifier();

    virtual kbpO_Profile* Copy() const;

};

#endif
