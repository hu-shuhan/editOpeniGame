//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kamO_ApiExtension_h
#define kamO_ApiExtension_h

// Begin local includes
#include <odb_ApiExtension.h>
#include <kamO_AmplitudeContainer.h>
#include <odb_Union.h>

// Forward declarations
class kamO_ActuatorAmplitude;
class kamO_DecayAmplitude;
class kamO_EquallySpacedAmplitude;
class kamO_ModulatedAmplitude;
class kamO_PeriodicAmplitude;
class kamO_SmoothStepAmplitude;
class kamO_SolutionDependentAmplitude;
class kamO_SpectrumAmplitude;
class kamO_TabularAmplitude;
class kamO_UserAmplitude;

class kamO_ApiExtension : public odb_ApiExtension
{
public:
    kamO_ApiExtension();
    virtual ~kamO_ApiExtension();

    kamO_ActuatorAmplitude& ActuatorAmplitude(const odb_String& name,
                                              odb_String timeSpan="STEP");

    kamO_DecayAmplitude& DecayAmplitude(const odb_String& name,
                                        double initial,
                                        double maximum,
                                        double start,
                                        double decayTime,
                                        odb_String timeSpan="STEP");

    kamO_EquallySpacedAmplitude& EquallySpacedAmplitude(const odb_String& name,
                                                        double fixedInterval,
                                                        const odb_SequenceDouble& data,
                                                        double begin=0,
                                                        odb_Union smooth="SOLVER_DEFAULT",
                                                        odb_String timeSpan="STEP");

    kamO_ModulatedAmplitude& ModulatedAmplitude(const odb_String& name,
                                                double initial,
                                                double magnitude,
                                                double start,
                                                double frequency1,
                                                double frequency2,
                                                odb_String timeSpan="STEP");

    kamO_PeriodicAmplitude& PeriodicAmplitude(const odb_String& name,
                                              double frequency,
                                              double start,
                                              double a_0,
                                              const odb_SequenceSequenceDouble& data,
                                              odb_String timeSpan="STEP");

    kamO_SmoothStepAmplitude& SmoothStepAmplitude(const odb_String& name,
                                                  const odb_SequenceSequenceDouble& data,
                                                  odb_String timeSpan="STEP");

    kamO_SolutionDependentAmplitude& SolutionDependentAmplitude(const odb_String& name,
                                                                double initial=1,
                                                                double minimum=0.1,
                                                                double maximum=1000,
                                                                odb_String timeSpan="STEP");

    kamO_SpectrumAmplitude& SpectrumAmplitude(const odb_String& name,
                                              const odb_String& method,
                                              const odb_SequenceSequenceDouble& data,
                                              odb_String specificationUnits="ACCELERATION",
                                              odb_String eventUnits="EVENT_ACCELERATION",
                                              odb_String solution="ABSOLUTE_VALUE",
                                              double timeIncrement=0.0,
                                              double gravity=1.0,
                                              bool criticalDamping=false,
                                              odb_String amplitude="",
                                              odb_String timeSpan="STEP");

    kamO_TabularAmplitude& TabularAmplitude(const odb_String& name,
                                            const odb_SequenceSequenceDouble& data,
                                            odb_Union smooth="SOLVER_DEFAULT",
                                            odb_String timeSpan="STEP");

    kamO_UserAmplitude& UserAmplitude(const odb_String& name,
                                      int numVariables,
                                      odb_String timeSpan="STEP");

    const odb_Amplitude& amplitude( const odb_String& name ) const;
    kamO_AmplitudeContainer& amplitudes();
    int numAmplitudes() const;

    static typ_typeTag TypeId();
    virtual typ_typeTag DynTypeId() const;

// Undocumented and unsupported
    void ConstructContainer();

private:
    kamO_AmplitudeContainer amplitudeCont;
};

#endif
