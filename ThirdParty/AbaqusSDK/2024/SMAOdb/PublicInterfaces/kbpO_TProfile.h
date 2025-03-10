//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbpO_TProfile_h
#define kbpO_TProfile_h

// Begin local includes
#include <kbpO_Profile.h>
#include <odb_Types.h>
// Forward declarations
class kbpC_TbeamProfile;

class kbpO_TProfile : public kbpO_Profile
{
public:
    kbpO_TProfile( const kbpC_BeamProfileShortcut& shortcut );
    kbpO_TProfile();

    kbpO_TProfile(double b,
                  double h,
                  double l,
                  double tf,
                  double tw);

    kbpO_TProfile( const kbpO_TProfile& copy );

    virtual ~kbpO_TProfile();
    double b() const;
    double h() const;
    double l() const;
    double tf() const;
    double tw() const;
    kbpO_TProfile& operator=( const kbpO_TProfile& rhs );

    const kbpC_TbeamProfile* GetPointer() const;

    static unsigned int typeIdentifier();

    virtual kbpO_Profile* Copy() const;

};

#endif
