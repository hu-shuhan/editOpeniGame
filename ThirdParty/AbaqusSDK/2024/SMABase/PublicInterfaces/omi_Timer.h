//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
///////////////////////////////////////////////////////////////////////////////
//
// File Name: omi_Timer.h
// 
// Description:
//
//    This file contains the defintion of the Timer class.
//
// Author: Bob Unnold and Uttam M. Narsu
// 
// Creation date: Tue Aug 18 1992
//
// Modified by Matt Dunbar
// Fri Oct 24 2003
// Provide better resolution for Windows.
//

// Users making use of the omi_Timer for very high resolution timing
// or doing any kind of nested timing should be aware of the time required
// to call the timer itself. In order to give a rough idea of the cost, 
// the following times for combined calls to timer.Start() and timer.Stop()
// give some idea of the cost.

// Windows 2000 - 1 GHz   - 2.0E-6 secs
// Linux - 3.06 GHZ       - 2.0E-6 secs
// IBM Power4 1.45 GHZ    - 4.0E-6 secs
// HPUX Itanium - 900 MHz - 1.0E-5 secs


#ifndef omi_Timer_h
#define omi_Timer_h

// Begin local includes
#include <mem_AllocationOperatorsTiny.h>

class omi_Timer  : public mem_AllocationOperatorsTiny
{

  public:
    omi_Timer();

    void Start();
    void Stop();
    void Reset();
    double GetElapsedTime() const;
    double GetTotalCpuTime() const;
	double GetElapsedStartTime() const;

  private:
    omi_Timer& operator==(const omi_Timer&) const;
    omi_Timer& operator!=(const omi_Timer&) const;

    enum {Off,On} status_;      // status (is the timer running)

    double elapsed_start_;    // elapsed starting time
    double total_cpu_start_;  // total cummultive cpu starting time

    double elapsed_time_;     // cummultive elapsed time
    double total_cpu_time_;   // total cummultive cpu time

#if defined _WINDOWS_SOURCE
    double counterFreq_;
#endif
};




#endif  // #ifdef omi_Timer_h



