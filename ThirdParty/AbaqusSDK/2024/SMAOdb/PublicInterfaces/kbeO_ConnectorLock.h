//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2015
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbeO_ConnectorLock_h
#define kbeO_ConnectorLock_h
///////////////////////////////////////////////////////////////////////////////
// kbeO_ConnectorLock
//

// Begin local includes
#include <odb_Types.h>
#include <kbeO_ConnectorBehaviorOption.h>
#include <odb_Union.h>
// Forward declarations
class kbeC_ConnectorLock;

class kbeO_ConnectorLock: public kbeO_ConnectorBehaviorOption
{
  public:
    kbeO_ConnectorLock( const kbeC_ConnectorBehaviorOptionShortcut& shortcut );
    kbeO_ConnectorLock( odb_Union lockingComponent = "ALL",
                        odb_Union minMotion = "NONE",
                        odb_Union maxMotion = "NONE",
                        odb_Union minForce = "NONE",
                        odb_Union maxForce = "NONE",
                        const odb_SequenceInt& components = odb_SequenceInt() );
    kbeO_ConnectorLock( const kbeO_ConnectorLock& copy );
    virtual kbeO_ConnectorBehaviorOption* Copy() const;
    kbeO_ConnectorLock& operator=( const kbeO_ConnectorLock& rhs );

    virtual ~kbeO_ConnectorLock();
    odb_Union lockingComponent() const;
    odb_Union minMotion() const;
    odb_Union maxMotion() const;
    odb_Union minForce() const;
    odb_Union maxForce() const;
    odb_SequenceInt components() const;
    const kbeC_ConnectorLock* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kbeO_ConnectorLock_h
