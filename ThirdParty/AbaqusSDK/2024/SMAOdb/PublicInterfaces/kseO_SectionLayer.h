//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseO_SectionLayer_h
#define kseO_SectionLayer_h
///////////////////////////////////////////////////////////////////////////////
// kseO_SectionLayer
//

// Begin local includes
#include <cow_ArrayCOW.h>
#include <odb_Types.h>
#include <mem_AllocationOperators.h>
#include <kseC_LayerShortcut.h>
#include <odb_Union.h>
#include <odb_String.h>
#include <odb_DatumCsys.h>
// Forward declarations
class kseC_Layer;
typedef cow_CountedHld<kseC_Layer> kseC_LayerHld;

class kseO_SectionLayer: public mem_AllocationOperators
{
  public:
    kseO_SectionLayer( const kseC_LayerShortcut& shortcut );
    kseO_SectionLayer();
    kseO_SectionLayer( double thickness,
                       odb_String material,
                       odb_Union orientAngle = 0.0,
                       int numIntPts = 3,
                       odb_String axis = "AXIS_3",
                       double angle = 0.0,
                       odb_String additionalRotationType = "ROTATION_NONE",
                       odb_String plyName = """",
                       odb_DatumCsys orientation = odb_DatumCsys(),
                       odb_String additionalRotationField = """" );
    kseO_SectionLayer( const kseO_SectionLayer& copy );
    virtual kseO_SectionLayer* Copy() const;
    kseO_SectionLayer& operator=( const kseO_SectionLayer& rhs );

    virtual ~kseO_SectionLayer();
    double thickness() const;
    odb_Union orientAngle() const;
    int numIntPts() const;
    odb_String axis() const;
    double angle() const;
    odb_String additionalRotationType() const;
    odb_String material() const;
    odb_String plyName() const;
    odb_DatumCsys orientation() const;
    odb_String additionalRotationField() const;
    void SetPointer( kseC_Layer* ptr) { pointer = ptr; }
    const kseC_Layer* GetPointer() const;
    kseC_LayerShortcut Shortcut() const { return shortcut; } 

    bool hasValue() const { return shortcut.HasValue(); }

    static unsigned int typeIdentifier();
    unsigned int subTypeIdentifier() const;

protected:
    kseC_LayerShortcut shortcut;
    kseC_LayerHld pointer;
};

COW_ARRAYCOW2_DECL(kseO_SectionLayer, cow_Virtual);

#endif // kseO_SectionLayer_h
