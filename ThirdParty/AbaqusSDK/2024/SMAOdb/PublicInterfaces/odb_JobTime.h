//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_JobTime_h
#define odb_JobTime_h

// Begin local includes
#include <mem_AllocationOperatorsTiny.h>

class odb_JobTime : public mem_AllocationOperatorsTiny
{
public:
   double userTime();
   double systemTime();
   double wallclockTime();
   
private:
   double m_userTime;
   double m_systemTime;
   double m_wallclockTime;

public:
   odb_JobTime();
   odb_JobTime(double userTime, double systemTime, double wallclockTime);
};

#endif
