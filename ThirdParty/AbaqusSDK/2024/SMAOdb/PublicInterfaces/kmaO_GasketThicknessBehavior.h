//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_GasketThicknessBehavior_h
#define kmaO_GasketThicknessBehavior_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_GasketThicknessBehavior
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaO_ContactArea;
class kmaC_ThicknessBehavior;

class kmaO_GasketThicknessBehavior: public kmaO_MaterialOption
{
  public:
    kmaO_GasketThicknessBehavior( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_GasketThicknessBehavior();
    kmaO_GasketThicknessBehavior( const odb_SequenceSequenceDouble& table,
                                  bool temperatureDependency = false,
                                  int dependencies = 0,
                                  double tensileStiffnessFactor = 0.0010,
                                  odb_String type = "ELASTIC_PLASTIC",
                                  int unloadingDependencies = 0,
                                  bool unloadingTemperatureDependency = false,
                                  odb_String variableUnits = "STRESS",
                                  double yieldOnset = 0.1,
                                  odb_String yieldOnsetMethod = "RELATIVE_SLOPE_DROP",
                                  const odb_SequenceSequenceDouble& unloadingTable = odb_SequenceSequenceDouble() );
    kmaO_GasketThicknessBehavior( const kmaO_GasketThicknessBehavior& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_GasketThicknessBehavior& operator=( const kmaO_GasketThicknessBehavior& rhs );

    virtual ~kmaO_GasketThicknessBehavior();
    bool temperatureDependency() const;
    int dependencies() const;
    double tensileStiffnessFactor() const;
    odb_String type() const;
    int unloadingDependencies() const;
    bool unloadingTemperatureDependency() const;
    odb_String variableUnits() const;
    double yieldOnset() const;
    odb_String yieldOnsetMethod() const;
    odb_SequenceSequenceDouble table() const;
    odb_SequenceSequenceDouble unloadingTable() const;
    kmaO_ContactArea contactArea() const;
    kmaO_ContactArea ContactArea( const odb_SequenceSequenceDouble& table,
                                  bool temperatureDependency = false,
                                  int dependencies = 0 );
    const kmaC_ThicknessBehavior* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_GasketThicknessBehavior_h
