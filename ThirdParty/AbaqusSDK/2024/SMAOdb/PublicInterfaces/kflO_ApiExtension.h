//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kflO_ApiExtension_h
#define kflO_ApiExtension_h

// Begin local includes
#include <odb_ApiExtension.h>
#include <kflO_FilterContainer.h>
#include <odb_Union.h>

// Forward declarations
class kflO_ButterworthFilter;
class kflO_Chebyshev1Filter;
class kflO_Chebyshev2Filter;
class kflO_OperatorFilter;

class kflO_ApiExtension : public odb_ApiExtension
{
public:
    kflO_ApiExtension();
    virtual ~kflO_ApiExtension();

    kflO_ButterworthFilter& ButterworthFilter(const odb_String& name,
                                              double cutoffFrequency,
                                              int order=2,
                                              odb_String operation="NONE",
                                              bool halt=false,
                                              odb_Union limit="NONE",
                                              odb_String invariant="NONE");

    kflO_Chebyshev1Filter& Chebyshev1Filter(const odb_String& name,
                                            double cutoffFrequency,
                                            double rippleFactor=0.225,
                                            int order=2,
                                            odb_String operation="NONE",
                                            bool halt=false,
                                            odb_Union limit="NONE",
                                            odb_String invariant="NONE");

    kflO_Chebyshev2Filter& Chebyshev2Filter(const odb_String& name,
                                            double cutoffFrequency,
                                            double rippleFactor=0.025,
                                            int order=2,
                                            odb_String operation="NONE",
                                            bool halt=false,
                                            odb_Union limit="NONE",
                                            odb_String invariant="NONE");

    kflO_OperatorFilter& OperatorFilter(const odb_String& name,
                                        double cutoffFrequency,
                                        int order=2,
                                        odb_String operation="NONE",
                                        bool halt=false,
                                        odb_Union limit="NONE",
                                        odb_String invariant="NONE");

    const kflO_Filter& filter( const odb_String& name ) const;
    kflO_FilterContainer& filters();
    int numFilters() const;

    static typ_typeTag TypeId();
    virtual typ_typeTag DynTypeId() const;

// Undocumented and unsupported
    void ConstructContainer();

private:
    kflO_FilterContainer filterCont;
};

#endif
