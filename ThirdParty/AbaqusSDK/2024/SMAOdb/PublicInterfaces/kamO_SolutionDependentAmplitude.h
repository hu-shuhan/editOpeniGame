//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kamO_SolutionDependentAmplitude_h
#define kamO_SolutionDependentAmplitude_h

// Begin local includes
#include <odb_Amplitude.h>
#include <odb_Types.h>
#include <odb_String.h>
// Forward declarations
class kamC_SolutionDependent;

class kamO_SolutionDependentAmplitude : public odb_Amplitude
{
public:
    kamO_SolutionDependentAmplitude( const kamC_AmplitudeShortcut& shortcut );
    kamO_SolutionDependentAmplitude(double initial=1,
                                    double minimum=0.1,
                                    double maximum=1000,
                                    odb_String timeSpan="STEP");

    kamO_SolutionDependentAmplitude( const kamO_SolutionDependentAmplitude& copy );

    virtual ~kamO_SolutionDependentAmplitude();
    double initial() const;
    double minimum() const;
    double maximum() const;
    odb_String timeSpan() const;
    kamO_SolutionDependentAmplitude& operator=( const kamO_SolutionDependentAmplitude& rhs );

    const kamC_SolutionDependent* GetPointer() const;

    static unsigned int typeIdentifier();

    virtual odb_Amplitude* Copy() const;

};

#endif
