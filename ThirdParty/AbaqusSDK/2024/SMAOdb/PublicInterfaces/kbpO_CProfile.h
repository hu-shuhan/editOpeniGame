

//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbpO_CProfile_h
#define kbpO_CProfile_h

// Begin local includes
#include <kbpO_Profile.h>
#include <odb_Types.h>
// Forward declarations
class kbpC_CbeamProfile;

class kbpO_CProfile : public kbpO_Profile
{
public:
    kbpO_CProfile(const kbpC_BeamProfileShortcut& shortcut);
    kbpO_CProfile();

    kbpO_CProfile(double l,
        double h,
        double b1,
        double b2,
        double t1,
        double t2,
        double t3,
        double o);

    kbpO_CProfile(const kbpO_CProfile& copy);

    virtual ~kbpO_CProfile();
    double l() const;
    double h() const;
    double b1() const;
    double b2() const;
    double t1() const;
    double t2() const;
    double t3() const;
    double o() const;
    kbpO_CProfile& operator=(const kbpO_CProfile& rhs);

    const kbpC_CbeamProfile* GetPointer() const;

    static unsigned int typeIdentifier();

    virtual kbpO_Profile* Copy() const;

};

#endif



