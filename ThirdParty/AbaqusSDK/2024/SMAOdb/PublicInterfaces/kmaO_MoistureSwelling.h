//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_MoistureSwelling_h
#define kmaO_MoistureSwelling_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_MoistureSwelling
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaO_Ratios;
class kmaC_MoistureSwelling;

class kmaO_MoistureSwelling: public kmaO_MaterialOption
{
  public:
    kmaO_MoistureSwelling( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_MoistureSwelling();
    kmaO_MoistureSwelling( const odb_SequenceSequenceDouble& table );
    kmaO_MoistureSwelling( const kmaO_MoistureSwelling& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_MoistureSwelling& operator=( const kmaO_MoistureSwelling& rhs );

    virtual ~kmaO_MoistureSwelling();
    odb_SequenceSequenceDouble table() const;
    kmaO_Ratios ratios() const;
    kmaO_Ratios Ratios( const odb_SequenceSequenceDouble& table,
                        bool temperatureDependency = false,
                        int dependencies = 0 );
    const kmaC_MoistureSwelling* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_MoistureSwelling_h
