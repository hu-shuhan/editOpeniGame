//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kamO_EquallySpacedAmplitude_h
#define kamO_EquallySpacedAmplitude_h

// Begin local includes
#include <odb_Amplitude.h>
#include <odb_Types.h>
#include <odb_Union.h>
#include <odb_String.h>
// Forward declarations
class kamO_BaselineCorrection;
class kamC_EquallySpaced;

class kamO_EquallySpacedAmplitude : public odb_Amplitude
{
public:
    kamO_EquallySpacedAmplitude( const kamC_AmplitudeShortcut& shortcut );
    kamO_EquallySpacedAmplitude();

    kamO_EquallySpacedAmplitude(double fixedInterval,
                                const odb_SequenceDouble& data,
                                double begin=0,
                                odb_Union smooth="SOLVER_DEFAULT",
                                odb_String timeSpan="STEP");

    kamO_EquallySpacedAmplitude( const kamO_EquallySpacedAmplitude& copy );

    virtual ~kamO_EquallySpacedAmplitude();
    double fixedInterval() const;
    double begin() const;
    odb_Union smooth() const;
    odb_SequenceDouble data() const;
    odb_String timeSpan() const;
    kamO_EquallySpacedAmplitude& operator=( const kamO_EquallySpacedAmplitude& rhs );

    const kamC_EquallySpaced* GetPointer() const;

    kamO_BaselineCorrection BaselineCorrection(  odb_SequenceDouble intervals = odb_SequenceDouble() );

    kamO_BaselineCorrection baselineCorrection() const;

    static unsigned int typeIdentifier();

    virtual odb_Amplitude* Copy() const;

};

#endif
