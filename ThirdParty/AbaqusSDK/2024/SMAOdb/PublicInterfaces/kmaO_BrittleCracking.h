//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_BrittleCracking_h
#define kmaO_BrittleCracking_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_BrittleCracking
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaO_BrittleShear;
class kmaO_BrittleFailure;
class kmaC_BrittleCracking;

class kmaO_BrittleCracking: public kmaO_MaterialOption
{
  public:
    kmaO_BrittleCracking( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_BrittleCracking();
    kmaO_BrittleCracking( const odb_SequenceSequenceDouble& table,
                          bool temperatureDependency = false,
                          int dependencies = 0,
                          odb_String type = "STRAIN" );
    kmaO_BrittleCracking( const kmaO_BrittleCracking& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_BrittleCracking& operator=( const kmaO_BrittleCracking& rhs );

    virtual ~kmaO_BrittleCracking();
    bool temperatureDependency() const;
    int dependencies() const;
    odb_String type() const;
    odb_SequenceSequenceDouble table() const;
    kmaO_BrittleShear brittleShear() const;
    kmaO_BrittleFailure brittleFailure() const;
    kmaO_BrittleShear BrittleShear( const odb_SequenceSequenceDouble& table,
                                    bool temperatureDependency = false,
                                    int dependencies = 0,
                                    odb_String type = "RETENTION_FACTOR" );
    kmaO_BrittleFailure BrittleFailure( const odb_SequenceSequenceDouble& table,
                                        bool temperatureDependency = false,
                                        int dependencies = 0,
                                        odb_String failureCriteria = "UNIDIRECTIONAL" );
    const kmaC_BrittleCracking* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_BrittleCracking_h
