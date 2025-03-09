//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2015
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbeO_ConnectorFailure_h
#define kbeO_ConnectorFailure_h
///////////////////////////////////////////////////////////////////////////////
// kbeO_ConnectorFailure
//

// Begin local includes
#include <odb_Types.h>
#include <kbeO_ConnectorBehaviorOption.h>
#include <odb_Union.h>
// Forward declarations
class kbeC_ConnectorFailure;

class kbeO_ConnectorFailure: public kbeO_ConnectorBehaviorOption
{
  public:
    kbeO_ConnectorFailure( const kbeC_ConnectorBehaviorOptionShortcut& shortcut );
    kbeO_ConnectorFailure( odb_Union releaseComponent = "ALL",
                           odb_Union minMotion = "NONE",
                           odb_Union maxMotion = "NONE",
                           odb_Union minForce = "NONE",
                           odb_Union maxForce = "NONE",
                           const odb_SequenceInt& components = odb_SequenceInt() );
    kbeO_ConnectorFailure( const kbeO_ConnectorFailure& copy );
    virtual kbeO_ConnectorBehaviorOption* Copy() const;
    kbeO_ConnectorFailure& operator=( const kbeO_ConnectorFailure& rhs );

    virtual ~kbeO_ConnectorFailure();
    odb_Union releaseComponent() const;
    odb_Union minMotion() const;
    odb_Union maxMotion() const;
    odb_Union minForce() const;
    odb_Union maxForce() const;
    odb_SequenceInt components() const;
    const kbeC_ConnectorFailure* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kbeO_ConnectorFailure_h
