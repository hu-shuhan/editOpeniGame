//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_FilmCondition_h
#define odb_FilmCondition_h

// Begin local includes
#include <odb_Interaction.h>
#include <odb_Types.h>
#include <cow_COW.h>

// Forward declarations
class ddb_FilmCondition;
class odb_FilmConditionProp;
class odb_Amplitude;

class odb_FilmCondition: public odb_Interaction
{
public:

    void setInteractionPropertyInStep( const odb_String& stepName,
				       const odb_FilmConditionProp& p );
    
    void setSinkTemperatureInStep( const odb_String& stepName,
				   double t );

    void setSinkAmplitudeInStep( const odb_String& stepName,
				 const odb_Amplitude& amp );

    void setFilmCoeffInStep( const odb_String& stepName,
			     double fc );
    
    void setFilmCoeffAmplitudeInStep( const odb_String& stepName,
				      const odb_Amplitude& amp );

private:

public:   // Undocumented and unsupported

    // Needed for container
    odb_FilmCondition();

    odb_FilmCondition(const ddb_FilmCondition& interaction);

};

#endif // odb_FilmCondition_h
