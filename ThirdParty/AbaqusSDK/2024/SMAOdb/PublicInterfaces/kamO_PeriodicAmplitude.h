//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kamO_PeriodicAmplitude_h
#define kamO_PeriodicAmplitude_h

// Begin local includes
#include <odb_Amplitude.h>
#include <odb_Types.h>
#include <odb_String.h>
// Forward declarations
class kamC_Periodic;

class kamO_PeriodicAmplitude : public odb_Amplitude
{
public:
    kamO_PeriodicAmplitude( const kamC_AmplitudeShortcut& shortcut );
    kamO_PeriodicAmplitude();

    kamO_PeriodicAmplitude(double frequency,
                           double start,
                           double a_0,
                           const odb_SequenceSequenceDouble& data,
                           odb_String timeSpan="STEP");

    kamO_PeriodicAmplitude( const kamO_PeriodicAmplitude& copy );

    virtual ~kamO_PeriodicAmplitude();
    double frequency() const;
    double start() const;
    double a_0() const;
    odb_SequenceSequenceDouble data() const;
    odb_String timeSpan() const;
    kamO_PeriodicAmplitude& operator=( const kamO_PeriodicAmplitude& rhs );

    const kamC_Periodic* GetPointer() const;

    static unsigned int typeIdentifier();

    virtual odb_Amplitude* Copy() const;

};

#endif
