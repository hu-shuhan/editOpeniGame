//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ddb_OutputRequest_h
#define ddb_OutputRequest_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <ddr_ModelShortcut.h>
#include <cow_ListInt.h>
#include <cow_ListString.h>

// Forward declarations
class ddb_Set;

template <class T1, class T2> class ddb_OutputRequest  : public mem_AllocationOperators
{
public:
    ddb_OutputRequest( const T1& sc,
		       const ddr_ModelShortcut& model );
    ddb_OutputRequest();
    ~ddb_OutputRequest();

    void deactivate( const cow_String &stepName );

    ddb_Set region() const;
    void setRegion( const ddb_Set& r );

    const cow_String& boltLoad() const;
    void setBoltLoad( const cow_String& bl );

    const cow_ListInt& sectionPoints() const;
    void setSectionPoints( const cow_ListInt& sp );

    const cow_ListString& connectors() const;
    void setConnectors( const cow_ListString& c );

    void setFrequency( int f );
    void setFrequencyInStep( const cow_String& stepName,
			     int f );

    void setNumIntervals( int ni );
    void setNumIntervalsInStep( const cow_String& stepName,
				int ni );

    void setTimeInterval( double ti );
    void setTimeIntervalInStep( const cow_String& stepName,
				double ti );

    void setModes( const cow_ListInt& m );
    void setModesInStep( const cow_String& stepName,
			 const cow_ListInt& m );

    void setVariables( const cow_ListString& vars );
    void setVariablesInStep( const cow_String& stepName,
			     const cow_ListString& vars );

    void setTimeMarks( bool tm );
    void setTimeMarksInStep( const cow_String& stepName,
			     bool tm );

    void assertValid() const;

    T1 Shortcut() const { return shortcut; }

private:
    T1 shortcut;
    ddr_ModelShortcut model;
};

#define ddb_OUTPUT_REQUEST_DECL( CLASS1, CLASS2, NAME )\
typedef ddb_OutputRequest<CLASS1,CLASS2> NAME;

#define ddb_OUTPUT_REQUEST_IMPL( CLASS1, CLASS2, NAME)\
template class ddb_OutputRequest<CLASS1,CLASS2>;

#endif
