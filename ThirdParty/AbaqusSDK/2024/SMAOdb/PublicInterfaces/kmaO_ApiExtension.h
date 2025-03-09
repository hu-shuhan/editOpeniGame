//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_ApiExtension_h
#define kmaO_ApiExtension_h

// Begin local includes
#include <odb_ApiExtension.h>
#include <kmaO_MaterialContainer.h>

// Forward declarations
class kmaO_Material;

class kmaO_ApiExtension : public odb_ApiExtension
{
public:
    kmaO_ApiExtension();
    virtual ~kmaO_ApiExtension();

    kmaO_Material& Material(const odb_String& name,
                            odb_String description="",
                            odb_String materialIdentifier="");

    const kmaO_Material& material( const odb_String& name ) const;
    kmaO_MaterialContainer& materials();
    int numMaterials() const;

    static typ_typeTag TypeId();
    virtual typ_typeTag DynTypeId() const;

// Undocumented and unsupported
    void ConstructContainer();

private:
    kmaO_MaterialContainer materialCont;
};

#endif
