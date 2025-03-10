//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseO_ConnectorSection_h
#define kseO_ConnectorSection_h
///////////////////////////////////////////////////////////////////////////////
// kseO_ConnectorSection
//

// Begin local includes
#include <odb_Types.h>
#include <odb_Section.h>
#include <odb_String.h>
#include <odb_Union.h>
#include <kbeO_SequenceBehaviorOption.h>
// Forward declarations
class kseC_ConnectorSection;

class kseO_ConnectorSection: public odb_Section
{
  public:
    kseO_ConnectorSection( const kseC_SectionShortcut& shortcut );
    kseO_ConnectorSection();
    kseO_ConnectorSection( const kseO_ConnectorSection& copy );
    virtual odb_Section* Copy() const;
    kseO_ConnectorSection& operator=( const kseO_ConnectorSection& rhs );

    virtual ~kseO_ConnectorSection();
    odb_String assembledType() const;
    odb_String rotationalType() const;
    odb_String translationalType() const;
    odb_String integration() const;
    odb_Union u1ReferenceLength() const;
    odb_Union u2ReferenceLength() const;
    odb_Union u3ReferenceLength() const;
    odb_Union ur1ReferenceAngle() const;
    odb_Union ur2ReferenceAngle() const;
    odb_Union ur3ReferenceAngle() const;
    odb_Union massPerLength() const;
    odb_Union contactAngle() const;
    double materialFlowFactor() const;
    bool regularize() const;
    bool defaultTolerance() const;
    double regularization() const;
    odb_String extrapolation() const;
    kbeO_SequenceConnectorBehaviorOption behaviorOptions() const;
    static unsigned int typeIdentifier();
};

#endif // kseO_ConnectorSection_h
