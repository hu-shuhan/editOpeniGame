//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseO_PointSection_h
#define kseO_PointSection_h
///////////////////////////////////////////////////////////////////////////////
// kseO_PointSection
//

// Begin local includes
#include <odb_Types.h>
#include <odb_Section.h>
// Forward declarations
class kseC_PointSection;

class kseO_PointSection: public odb_Section
{
  public:
    kseO_PointSection( const kseC_SectionShortcut& shortcut );
    kseO_PointSection();
    kseO_PointSection( const kseO_PointSection& copy );
    virtual odb_Section* Copy() const;
    kseO_PointSection& operator=( const kseO_PointSection& rhs );

    virtual ~kseO_PointSection();
    double mass() const;
    double i11() const;
    double i12() const;
    double i13() const;
    double i22() const;
    double i23() const;
    double i33() const;
    double massAlphaDamping() const;
    double massCompositeDamping() const;
    double rotaryAlphaDamping() const;
    double rotaryCompositeDamping() const;
    double heatCapacitance() const;
    static unsigned int typeIdentifier();
};

#endif // kseO_PointSection_h
