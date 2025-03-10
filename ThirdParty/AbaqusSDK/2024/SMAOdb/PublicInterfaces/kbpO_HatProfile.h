

//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbpO_HatProfile_h
#define kbpO_HatProfile_h

// Begin local includes
#include <kbpO_Profile.h>
#include <odb_Types.h>
// Forward declarations
class kbpC_HatProfile;

class kbpO_HatProfile : public kbpO_Profile
{
public:
    kbpO_HatProfile(const kbpC_BeamProfileShortcut& shortcut);
    kbpO_HatProfile();

    kbpO_HatProfile(double l,
        double h,
        double b,
        double b1,
        double b2,
        double t1,
        double t2,
        double t3);

    kbpO_HatProfile(const kbpO_HatProfile& copy);

    virtual ~kbpO_HatProfile();
    double l() const;
    double h() const;
    double b() const;
    double b1() const;
    double b2() const;
    double t1() const;
    double t2() const;
    double t3() const;
    kbpO_HatProfile& operator=(const kbpO_HatProfile& rhs);

    const kbpC_HatProfile* GetPointer() const;

    static unsigned int typeIdentifier();

    virtual kbpO_Profile* Copy() const;

};

#endif


