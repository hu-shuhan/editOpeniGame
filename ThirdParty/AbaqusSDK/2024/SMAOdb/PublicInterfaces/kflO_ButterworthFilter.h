//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kflO_ButterworthFilter_h
#define kflO_ButterworthFilter_h

// Begin local includes
#include <kflO_Filter.h>
#include <odb_Types.h>
#include <odb_String.h>
#include <odb_Union.h>
// Forward declarations
class kflC_ButterworthFilter;

class kflO_ButterworthFilter : public kflO_Filter
{
public:
    kflO_ButterworthFilter( const kflC_FilterShortcut& shortcut );
    kflO_ButterworthFilter();

    kflO_ButterworthFilter(double cutoffFrequency,
                           int order=2,
                           odb_String operation="NONE",
                           bool halt=false,
                           odb_Union limit="NONE",
                           odb_String invariant="NONE");

    kflO_ButterworthFilter( const kflO_ButterworthFilter& copy );

    virtual ~kflO_ButterworthFilter();
    double cutoffFrequency() const;
    int order() const;
    odb_String operation() const;
    bool halt() const;
    odb_Union limit() const;
    odb_String invariant() const;
    kflO_ButterworthFilter& operator=( const kflO_ButterworthFilter& rhs );

    const kflC_ButterworthFilter* GetPointer() const;

    static unsigned int typeIdentifier();

    virtual kflO_Filter* Copy() const;

};

#endif
