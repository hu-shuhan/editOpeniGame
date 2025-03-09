//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2015
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbeO_ConnectorElasticity_h
#define kbeO_ConnectorElasticity_h
///////////////////////////////////////////////////////////////////////////////
// kbeO_ConnectorElasticity
//

// Begin local includes
#include <odb_Types.h>
#include <kbeO_ConnectorBehaviorOption.h>
#include <odb_String.h>
// Forward declarations
class kbeO_ConnectorOptions;
class kbeC_ConnectorElasticity;

class kbeO_ConnectorElasticity: public kbeO_ConnectorBehaviorOption
{
  public:
    kbeO_ConnectorElasticity( const kbeC_ConnectorBehaviorOptionShortcut& shortcut );
    kbeO_ConnectorElasticity( odb_String behavior = "LINEAR",
                              odb_String coupling = "UNCOUPLED",
                              int dependencies = 0,
                              bool temperatureDependency = false,
                              bool frequencyDependency = false,
                              const odb_SequenceSequenceDouble& table = odb_SequenceSequenceDouble(),
                              const odb_SequenceInt& independentComponents = odb_SequenceInt(),
                              const odb_SequenceInt& components = odb_SequenceInt() );
    kbeO_ConnectorElasticity( const kbeO_ConnectorElasticity& copy );
    virtual kbeO_ConnectorBehaviorOption* Copy() const;
    kbeO_ConnectorElasticity& operator=( const kbeO_ConnectorElasticity& rhs );

    virtual ~kbeO_ConnectorElasticity();
    odb_String behavior() const;
    odb_String coupling() const;
    int dependencies() const;
    bool temperatureDependency() const;
    bool frequencyDependency() const;
    odb_SequenceSequenceDouble table() const;
    odb_SequenceInt independentComponents() const;
    odb_SequenceInt components() const;
    kbeO_ConnectorOptions options() const;
    kbeO_ConnectorOptions Options( bool useBehRegSettings = true,
                                   bool regularize = true,
                                   bool defaultTolerance = true,
                                   double regularization = 0.03,
                                   bool defaultRateFactor = true,
                                   double rateFactor = 0.9,
                                   odb_String interpolation = "LINEAR",
                                   bool useBehExtSettings = true,
                                   odb_String extrapolation = "CONSTANT" );
    const kbeC_ConnectorElasticity* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kbeO_ConnectorElasticity_h
