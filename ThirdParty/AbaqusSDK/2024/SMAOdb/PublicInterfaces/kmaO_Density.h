//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_Density_h
#define kmaO_Density_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_Density
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaC_Density;

class kmaO_Density: public kmaO_MaterialOption
{
  public:
    kmaO_Density( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_Density();
    kmaO_Density( const odb_SequenceSequenceDouble& table,
                  bool temperatureDependency = false,
                  int dependencies = 0,
                  odb_String distributionType = "UNIFORM",
                  odb_String fieldName = "" );
    kmaO_Density( const kmaO_Density& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_Density& operator=( const kmaO_Density& rhs );

    virtual ~kmaO_Density();
    bool temperatureDependency() const;
    int dependencies() const;
    odb_String distributionType() const;
    odb_String fieldName() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_Density* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_Density_h
