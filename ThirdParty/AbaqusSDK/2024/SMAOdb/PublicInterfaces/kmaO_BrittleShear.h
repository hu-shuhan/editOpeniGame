//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_BrittleShear_h
#define kmaO_BrittleShear_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_BrittleShear
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaC_BrittleShear;

class kmaO_BrittleShear: public kmaO_MaterialOption
{
  public:
    kmaO_BrittleShear( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_BrittleShear();
    kmaO_BrittleShear( const odb_SequenceSequenceDouble& table,
                       bool temperatureDependency = false,
                       int dependencies = 0,
                       odb_String type = "RETENTION_FACTOR" );
    kmaO_BrittleShear( const kmaO_BrittleShear& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_BrittleShear& operator=( const kmaO_BrittleShear& rhs );

    virtual ~kmaO_BrittleShear();
    bool temperatureDependency() const;
    int dependencies() const;
    odb_String type() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_BrittleShear* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_BrittleShear_h
