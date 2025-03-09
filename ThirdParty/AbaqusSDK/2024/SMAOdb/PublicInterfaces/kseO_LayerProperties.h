//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseO_LayerProperties_h
#define kseO_LayerProperties_h
///////////////////////////////////////////////////////////////////////////////
// kseO_LayerProperties
//

// Begin local includes
#include <cow_ArrayCOW.h>
#include <odb_Types.h>
#include <mem_AllocationOperators.h>
#include <kseC_LayerPropertiesShortcut.h>
#include <odb_Union.h>
// Forward declarations
class kseC_LayerProperties;
typedef cow_CountedHld<kseC_LayerProperties> kseC_LayerPropertiesHld;

class kseO_LayerProperties: public mem_AllocationOperators
{
  public:
    kseO_LayerProperties( const kseC_LayerPropertiesShortcut& shortcut );
    kseO_LayerProperties();
    kseO_LayerProperties( double barArea,
                          odb_Union orientationAngle,
                          odb_String layerName,
                          odb_String material,
                          double barSpacing = 0,
                          double layerPosition = 0,
                          double spacingAngle = 0,
                          double extensionRatio = 0,
                          double radius = 0 );
    kseO_LayerProperties( const kseO_LayerProperties& copy );
    virtual kseO_LayerProperties* Copy() const;
    kseO_LayerProperties& operator=( const kseO_LayerProperties& rhs );

    virtual ~kseO_LayerProperties();
    double barArea() const;
    odb_Union orientationAngle() const;
    double barSpacing() const;
    double layerPosition() const;
    double spacingAngle() const;
    double extensionRatio() const;
    double radius() const;
    odb_String layerName() const;
    odb_String material() const;
    void SetPointer( kseC_LayerProperties* ptr) { pointer = ptr; }
    const kseC_LayerProperties* GetPointer() const;
    kseC_LayerPropertiesShortcut Shortcut() const { return shortcut; } 

    bool hasValue() const { return shortcut.HasValue(); }

    static unsigned int typeIdentifier();
    unsigned int subTypeIdentifier() const;

protected:
    kseC_LayerPropertiesShortcut shortcut;
    kseC_LayerPropertiesHld pointer;
};

COW_ARRAYCOW2_DECL(kseO_LayerProperties, cow_Virtual);

#endif // kseO_LayerProperties_h
