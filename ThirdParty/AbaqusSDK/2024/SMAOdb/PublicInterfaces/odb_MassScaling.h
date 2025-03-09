//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_MassScaling_h
#define odb_MassScaling_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_String.h>
#include <odb_Set.h>
#include <ddb_MassScaling.h>

// Forward declarations

class odb_MassScaling : public mem_AllocationOperators
{
public:
    odb_MassScaling(const odb_String objective="SEMI_AUTOMATIC",
		    const odb_Set& region=odb_Set(),
		    const odb_String occurs="AT_BEGINNING",
		    const odb_String type="BELOW_MIN",
		    double factor=0.0,
		    double dt=0.0,
		    int frequency=0,
		    int numberInterval=0,
		    double feedRate=0.0,
		    double extrudedLength=0.0,
		    int crossSection=0,
		    const odb_String direction="GLOBAL_X");
		    
    


private:
    ddb_MassScaling ms;

public:   //undocumented and unsupported
    ddb_MassScaling massScaling() const;

};

class odb_MassScalingList : public mem_AllocationOperators
{
public:
    odb_MassScalingList(){};
    
    void append(const odb_MassScaling& ms);

private:
    ddb_MassScalingList msList;

public:   //undocumented and unsupported
    kprC_MassScalingList massScalingList() const;
};

#endif // odb_MassScaling_h

