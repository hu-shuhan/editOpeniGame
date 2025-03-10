//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kflO_Chebyshev2Filter_h
#define kflO_Chebyshev2Filter_h

// Begin local includes
#include <kflO_Filter.h>
#include <odb_Types.h>
#include <odb_String.h>
#include <odb_Union.h>
// Forward declarations
class kflC_Chebyshev2Filter;

class kflO_Chebyshev2Filter : public kflO_Filter
{
public:
    kflO_Chebyshev2Filter( const kflC_FilterShortcut& shortcut );
    kflO_Chebyshev2Filter();

    kflO_Chebyshev2Filter(double cutoffFrequency,
                          double rippleFactor=0.025,
                          int order=2,
                          odb_String operation="NONE",
                          bool halt=false,
                          odb_Union limit="NONE",
                          odb_String invariant="NONE");

    kflO_Chebyshev2Filter( const kflO_Chebyshev2Filter& copy );

    virtual ~kflO_Chebyshev2Filter();
    double rippleFactor() const;
    double cutoffFrequency() const;
    int order() const;
    odb_String operation() const;
    bool halt() const;
    odb_Union limit() const;
    odb_String invariant() const;
    kflO_Chebyshev2Filter& operator=( const kflO_Chebyshev2Filter& rhs );

    const kflC_Chebyshev2Filter* GetPointer() const;

    static unsigned int typeIdentifier();

    virtual kflO_Filter* Copy() const;

};

#endif
