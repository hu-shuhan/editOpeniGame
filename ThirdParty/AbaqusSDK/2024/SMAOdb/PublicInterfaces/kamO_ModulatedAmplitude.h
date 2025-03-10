//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kamO_ModulatedAmplitude_h
#define kamO_ModulatedAmplitude_h

// Begin local includes
#include <odb_Amplitude.h>
#include <odb_Types.h>
#include <odb_String.h>
// Forward declarations
class kamC_Modulated;

class kamO_ModulatedAmplitude : public odb_Amplitude
{
public:
    kamO_ModulatedAmplitude( const kamC_AmplitudeShortcut& shortcut );
    kamO_ModulatedAmplitude();

    kamO_ModulatedAmplitude(double initial,
                            double magnitude,
                            double start,
                            double frequency1,
                            double frequency2,
                            odb_String timeSpan="STEP");

    kamO_ModulatedAmplitude( const kamO_ModulatedAmplitude& copy );

    virtual ~kamO_ModulatedAmplitude();
    double initial() const;
    double magnitude() const;
    double start() const;
    double frequency1() const;
    double frequency2() const;
    odb_String timeSpan() const;
    kamO_ModulatedAmplitude& operator=( const kamO_ModulatedAmplitude& rhs );

    const kamC_Modulated* GetPointer() const;

    static unsigned int typeIdentifier();

    virtual odb_Amplitude* Copy() const;

};

#endif
