//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_GapFlow_h
#define kmaO_GapFlow_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_GapFlow
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_Union.h>
#include <odb_String.h>
// Forward declarations
class kmaC_GapFlow;

class kmaO_GapFlow: public kmaO_MaterialOption
{
  public:
    kmaO_GapFlow( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_GapFlow();
    kmaO_GapFlow( const odb_SequenceSequenceDouble& table,
                  odb_Union kmax = "NONE",
                  bool temperatureDependency = false,
                  int dependencies = 0,
                  odb_String type = "NEWTONIAN" );
    kmaO_GapFlow( const kmaO_GapFlow& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_GapFlow& operator=( const kmaO_GapFlow& rhs );

    virtual ~kmaO_GapFlow();
    odb_Union kmax() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_String type() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_GapFlow* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_GapFlow_h
