//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ddb_MassScaling_h
#define ddb_MassScaling_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <kprC_MassScaling.h>
#include <ddb_Set.h>

//Forward declarations
class ddb_MassScaling;
class ddb_Set;

class ddb_MassScaling  : public mem_AllocationOperators
{
public:
    ddb_MassScaling(){};

    ~ddb_MassScaling(){};

    kprC_MassScaling massScaling() const;

    cow_String objective, occurs, type, direction;
    double factor, dt;
    int frequency, numberInterval, crossSection;
    double feedRate, extrudedLength;
    ddb_Set rgn;
    
private:
     void SetEnum(const cow_String& enumName,
		  omu_PrimEnumBase& enm,
		  const cow_String& typeFound) const;
};

COW_LIST_DECL(ddb_MassScaling, ddb_MassScalingList);

#endif 
