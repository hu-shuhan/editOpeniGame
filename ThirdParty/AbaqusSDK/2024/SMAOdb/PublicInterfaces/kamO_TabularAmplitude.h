//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kamO_TabularAmplitude_h
#define kamO_TabularAmplitude_h

// Begin local includes
#include <odb_Amplitude.h>
#include <odb_Types.h>
#include <odb_Union.h>
#include <odb_String.h>
// Forward declarations
class kamO_BaselineCorrection;
class kamC_Tabular;

class kamO_TabularAmplitude : public odb_Amplitude
{
public:
    kamO_TabularAmplitude( const kamC_AmplitudeShortcut& shortcut );
    kamO_TabularAmplitude();

    kamO_TabularAmplitude(const odb_SequenceSequenceDouble& data,
                          odb_Union smooth="SOLVER_DEFAULT",
                          odb_String timeSpan="STEP");

    kamO_TabularAmplitude( const kamO_TabularAmplitude& copy );

    virtual ~kamO_TabularAmplitude();
    odb_Union smooth() const;
    odb_SequenceSequenceDouble data() const;
    odb_String timeSpan() const;
    kamO_TabularAmplitude& operator=( const kamO_TabularAmplitude& rhs );

    const kamC_Tabular* GetPointer() const;

    kamO_BaselineCorrection BaselineCorrection(  odb_SequenceDouble intervals = odb_SequenceDouble() );

    kamO_BaselineCorrection baselineCorrection() const;

    static unsigned int typeIdentifier();

    virtual odb_Amplitude* Copy() const;

};

#endif
