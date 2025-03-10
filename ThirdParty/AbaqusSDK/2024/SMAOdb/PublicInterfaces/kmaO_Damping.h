//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_Damping_h
#define kmaO_Damping_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_Damping
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_Damping;

class kmaO_Damping: public kmaO_MaterialOption
{
  public:
    kmaO_Damping( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_Damping( double alpha = 0,
                  double beta = 0,
                  double composite = 0,
                  double structural = 0 );
    kmaO_Damping( const kmaO_Damping& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_Damping& operator=( const kmaO_Damping& rhs );

    virtual ~kmaO_Damping();
    double alpha() const;
    double beta() const;
    double composite() const;
    double structural() const;
    const kmaC_Damping* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_Damping_h
