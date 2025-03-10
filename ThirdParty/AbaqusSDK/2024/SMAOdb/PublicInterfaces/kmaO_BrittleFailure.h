//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_BrittleFailure_h
#define kmaO_BrittleFailure_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_BrittleFailure
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaC_BrittleFailure;

class kmaO_BrittleFailure: public kmaO_MaterialOption
{
  public:
    kmaO_BrittleFailure( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_BrittleFailure();
    kmaO_BrittleFailure( const odb_SequenceSequenceDouble& table,
                         bool temperatureDependency = false,
                         int dependencies = 0,
                         odb_String failureCriteria = "UNIDIRECTIONAL" );
    kmaO_BrittleFailure( const kmaO_BrittleFailure& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_BrittleFailure& operator=( const kmaO_BrittleFailure& rhs );

    virtual ~kmaO_BrittleFailure();
    bool temperatureDependency() const;
    int dependencies() const;
    odb_String failureCriteria() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_BrittleFailure* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_BrittleFailure_h
