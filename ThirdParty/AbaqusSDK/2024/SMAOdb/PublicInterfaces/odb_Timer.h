//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_Timer_h
#define odb_Timer_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <omi_Timer.h>
// End local includes

//
// Forward declarations
//

class odb_Timer : public mem_AllocationOperators
{
public:
    // documented and supported
    odb_Timer();
    ~odb_Timer();
    void Start();
    void Stop();
    void Reset();
    double GetElapsedTime() const;
    double GetTotalCpuTime() const;

private:
    odb_Timer( const odb_Timer& );
    odb_Timer& operator=( odb_Timer& );

private:
    // data
    omi_Timer _timer;
};

#endif // ifdef odb_Timer_h
