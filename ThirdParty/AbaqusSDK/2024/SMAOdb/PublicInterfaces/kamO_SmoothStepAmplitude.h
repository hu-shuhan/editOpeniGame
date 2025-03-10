//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kamO_SmoothStepAmplitude_h
#define kamO_SmoothStepAmplitude_h

// Begin local includes
#include <odb_Amplitude.h>
#include <odb_Types.h>
#include <odb_String.h>
// Forward declarations
class kamC_SmoothStep;

class kamO_SmoothStepAmplitude : public odb_Amplitude
{
public:
    kamO_SmoothStepAmplitude( const kamC_AmplitudeShortcut& shortcut );
    kamO_SmoothStepAmplitude();

    kamO_SmoothStepAmplitude(const odb_SequenceSequenceDouble& data,
                             odb_String timeSpan="STEP");

    kamO_SmoothStepAmplitude( const kamO_SmoothStepAmplitude& copy );

    virtual ~kamO_SmoothStepAmplitude();
    odb_SequenceSequenceDouble data() const;
    odb_String timeSpan() const;
    kamO_SmoothStepAmplitude& operator=( const kamO_SmoothStepAmplitude& rhs );

    const kamC_SmoothStep* GetPointer() const;

    static unsigned int typeIdentifier();

    virtual odb_Amplitude* Copy() const;

};

#endif
