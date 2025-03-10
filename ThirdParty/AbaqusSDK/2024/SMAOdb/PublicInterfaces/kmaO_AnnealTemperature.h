//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_AnnealTemperature_h
#define kmaO_AnnealTemperature_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_AnnealTemperature
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_AnnealTemperature;

class kmaO_AnnealTemperature: public kmaO_MaterialOption
{
  public:
    kmaO_AnnealTemperature( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_AnnealTemperature();
    kmaO_AnnealTemperature( const odb_SequenceSequenceDouble& table,
                            int dependencies = 0 );
    kmaO_AnnealTemperature( const kmaO_AnnealTemperature& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_AnnealTemperature& operator=( const kmaO_AnnealTemperature& rhs );

    virtual ~kmaO_AnnealTemperature();
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_AnnealTemperature* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_AnnealTemperature_h
