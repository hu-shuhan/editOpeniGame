//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kamO_DecayAmplitude_h
#define kamO_DecayAmplitude_h

// Begin local includes
#include <odb_Amplitude.h>
#include <odb_Types.h>
#include <odb_String.h>
// Forward declarations
class kamC_Decay;

class kamO_DecayAmplitude : public odb_Amplitude
{
public:
    kamO_DecayAmplitude( const kamC_AmplitudeShortcut& shortcut );
    kamO_DecayAmplitude();

    kamO_DecayAmplitude(double initial,
                        double maximum,
                        double start,
                        double decayTime,
                        odb_String timeSpan="STEP");

    kamO_DecayAmplitude( const kamO_DecayAmplitude& copy );

    virtual ~kamO_DecayAmplitude();
    double initial() const;
    double maximum() const;
    double start() const;
    double decayTime() const;
    odb_String timeSpan() const;
    kamO_DecayAmplitude& operator=( const kamO_DecayAmplitude& rhs );

    const kamC_Decay* GetPointer() const;

    static unsigned int typeIdentifier();

    virtual odb_Amplitude* Copy() const;

};

#endif
