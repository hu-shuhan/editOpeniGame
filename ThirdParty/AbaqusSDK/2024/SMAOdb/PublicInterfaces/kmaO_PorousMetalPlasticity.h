//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_PorousMetalPlasticity_h
#define kmaO_PorousMetalPlasticity_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_PorousMetalPlasticity
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_Union.h>
// Forward declarations
class kmaO_PorousFailureCriteria;
class kmaO_VoidNucleation;
class kmaC_PorousMetalPlasticity;

class kmaO_PorousMetalPlasticity: public kmaO_MaterialOption
{
  public:
    kmaO_PorousMetalPlasticity( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_PorousMetalPlasticity();
    kmaO_PorousMetalPlasticity( const odb_SequenceSequenceDouble& table,
                                odb_Union relativeDensity = "NONE",
                                bool temperatureDependency = false,
                                int dependencies = 0 );
    kmaO_PorousMetalPlasticity( const kmaO_PorousMetalPlasticity& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_PorousMetalPlasticity& operator=( const kmaO_PorousMetalPlasticity& rhs );

    virtual ~kmaO_PorousMetalPlasticity();
    odb_Union relativeDensity() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    kmaO_PorousFailureCriteria porousFailureCriteria() const;
    kmaO_VoidNucleation voidNucleation() const;
    kmaO_PorousFailureCriteria PorousFailureCriteria( double fraction = 1,
                                                      double criticalFraction = 1 );
    kmaO_VoidNucleation VoidNucleation( const odb_SequenceSequenceDouble& table,
                                        bool temperatureDependency = false,
                                        int dependencies = 0 );
    const kmaC_PorousMetalPlasticity* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_PorousMetalPlasticity_h
