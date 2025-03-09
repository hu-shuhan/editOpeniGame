//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_Piezoelectric_h
#define kmaO_Piezoelectric_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_Piezoelectric
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaC_Piezoelectric;

class kmaO_Piezoelectric: public kmaO_MaterialOption
{
  public:
    kmaO_Piezoelectric( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_Piezoelectric();
    kmaO_Piezoelectric( const odb_SequenceSequenceDouble& table,
                        odb_String type = "STRESS",
                        bool temperatureDependency = false,
                        int dependencies = 0 );
    kmaO_Piezoelectric( const kmaO_Piezoelectric& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_Piezoelectric& operator=( const kmaO_Piezoelectric& rhs );

    virtual ~kmaO_Piezoelectric();
    odb_String type() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_Piezoelectric* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_Piezoelectric_h
