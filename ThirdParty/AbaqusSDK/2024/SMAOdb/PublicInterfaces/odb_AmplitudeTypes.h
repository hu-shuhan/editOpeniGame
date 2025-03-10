//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_AmplitudeTypes_h
#define odb_AmplitudeTypes_h

// Begin local includes
#include <odb_Amplitude.h>
#include <kamO_DecayAmplitude.h>
#include <kamO_EquallySpacedAmplitude.h>
#include <kamO_ModulatedAmplitude.h>
#include <kamO_PeriodicAmplitude.h>
#include <kamO_SmoothStepAmplitude.h>
#include <kamO_SolutionDependentAmplitude.h>
#include <kamO_TabularAmplitude.h>
#include <kamO_BaselineCorrection.h>
#include <kamO_ApiExtension.h>
#include <kamO_AmplitudeContainer.h>

typedef kamO_DecayAmplitude odb_DecayAmplitude;
typedef kamO_EquallySpacedAmplitude odb_EquallySpacedAmplitude;
typedef kamO_ModulatedAmplitude odb_ModulatedAmplitude;
typedef kamO_PeriodicAmplitude odb_PeriodicAmplitude;
typedef kamO_SmoothStepAmplitude odb_SmoothStepAmplitude;
typedef kamO_SolutionDependentAmplitude odb_SolutionDependentAmplitude;
typedef kamO_TabularAmplitude odb_TabularAmplitude;
typedef kamO_ApiExtension odb_AmplitudeApi;
typedef kamO_AmplitudeContainer odb_AmplitudeContainer;
typedef kamO_AmplitudeContainerIT odb_AmplitudeContainerIT;
typedef kamO_BaselineCorrection odb_BaselineCorrection;

#endif

