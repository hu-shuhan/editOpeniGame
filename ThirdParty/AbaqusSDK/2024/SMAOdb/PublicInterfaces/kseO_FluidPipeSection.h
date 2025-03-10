//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseO_FluidPipeSection_h
#define kseO_FluidPipeSection_h
///////////////////////////////////////////////////////////////////////////////
// kseO_FluidPipeSection
//

// Begin local includes
#include <odb_Types.h>
#include <odb_Section.h>
#include <odb_String.h>
#include <odb_Union.h>
// Forward declarations
class kseO_TransverseShearFluidPipe;
class kseC_FluidPipeSection;

class kseO_FluidPipeSection: public odb_Section
{
  public:
    kseO_FluidPipeSection( const kseC_SectionShortcut& shortcut );
    kseO_FluidPipeSection();
    kseO_FluidPipeSection( const kseO_FluidPipeSection& copy );
    virtual odb_Section* Copy() const;
    kseO_FluidPipeSection& operator=( const kseO_FluidPipeSection& rhs );

    virtual ~kseO_FluidPipeSection();
    
    double innerRadius() const;
    double outerRadius() const;
    double laminarFlowTransition() const;
    odb_String type() const;    
    odb_String material() const;
    odb_SequenceDouble table() const;       
    static unsigned int typeIdentifier();
};

#endif // kseO_FluidPipeSection_h
