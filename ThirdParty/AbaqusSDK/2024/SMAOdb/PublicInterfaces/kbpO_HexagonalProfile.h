//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbpO_HexagonalProfile_h
#define kbpO_HexagonalProfile_h

// Begin local includes
#include <kbpO_Profile.h>
#include <odb_Types.h>
// Forward declarations
class kbpC_HexagonalProfile;

class kbpO_HexagonalProfile : public kbpO_Profile
{
public:
    kbpO_HexagonalProfile( const kbpC_BeamProfileShortcut& shortcut );
    kbpO_HexagonalProfile();

    kbpO_HexagonalProfile(double r,
                          double t);

    kbpO_HexagonalProfile( const kbpO_HexagonalProfile& copy );

    virtual ~kbpO_HexagonalProfile();
    double r() const;
    double t() const;
    kbpO_HexagonalProfile& operator=( const kbpO_HexagonalProfile& rhs );

    const kbpC_HexagonalProfile* GetPointer() const;

    static unsigned int typeIdentifier();

    virtual kbpO_Profile* Copy() const;

};

#endif
