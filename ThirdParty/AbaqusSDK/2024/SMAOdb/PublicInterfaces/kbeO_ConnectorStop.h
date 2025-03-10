//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2015
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbeO_ConnectorStop_h
#define kbeO_ConnectorStop_h
///////////////////////////////////////////////////////////////////////////////
// kbeO_ConnectorStop
//

// Begin local includes
#include <odb_Types.h>
#include <kbeO_ConnectorBehaviorOption.h>
#include <odb_Union.h>
// Forward declarations
class kbeC_ConnectorStop;

class kbeO_ConnectorStop: public kbeO_ConnectorBehaviorOption
{
  public:
    kbeO_ConnectorStop( const kbeC_ConnectorBehaviorOptionShortcut& shortcut );
    kbeO_ConnectorStop( odb_Union minMotion = "NONE",
                        odb_Union maxMotion = "NONE",
                        const odb_SequenceInt& components = odb_SequenceInt() );
    kbeO_ConnectorStop( const kbeO_ConnectorStop& copy );
    virtual kbeO_ConnectorBehaviorOption* Copy() const;
    kbeO_ConnectorStop& operator=( const kbeO_ConnectorStop& rhs );

    virtual ~kbeO_ConnectorStop();
    odb_Union minMotion() const;
    odb_Union maxMotion() const;
    odb_SequenceInt components() const;
    const kbeC_ConnectorStop* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kbeO_ConnectorStop_h
