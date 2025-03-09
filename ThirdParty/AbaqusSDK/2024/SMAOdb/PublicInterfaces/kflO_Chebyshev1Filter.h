//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kflO_Chebyshev1Filter_h
#define kflO_Chebyshev1Filter_h

// Begin local includes
#include <kflO_Filter.h>
#include <odb_Types.h>
#include <odb_String.h>
#include <odb_Union.h>
// Forward declarations
class kflC_Chebyshev1Filter;

class kflO_Chebyshev1Filter : public kflO_Filter
{
public:
    kflO_Chebyshev1Filter( const kflC_FilterShortcut& shortcut );
    kflO_Chebyshev1Filter();

    kflO_Chebyshev1Filter(double cutoffFrequency,
                          double rippleFactor=0.225,
                          int order=2,
                          odb_String operation="NONE",
                          bool halt=false,
                          odb_Union limit="NONE",
                          odb_String invariant="NONE");

    kflO_Chebyshev1Filter( const kflO_Chebyshev1Filter& copy );

    virtual ~kflO_Chebyshev1Filter();
    double rippleFactor() const;
    double cutoffFrequency() const;
    int order() const;
    odb_String operation() const;
    bool halt() const;
    odb_Union limit() const;
    odb_String invariant() const;
    kflO_Chebyshev1Filter& operator=( const kflO_Chebyshev1Filter& rhs );

    const kflC_Chebyshev1Filter* GetPointer() const;

    static unsigned int typeIdentifier();

    virtual kflO_Filter* Copy() const;

};

#endif
