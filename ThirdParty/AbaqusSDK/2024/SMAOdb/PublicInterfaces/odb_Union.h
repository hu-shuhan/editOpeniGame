//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_Union_h
#define odb_Union_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_String.h>

enum odb_UnionType {
    odb_UNION_INT,
    odb_UNION_FLOAT,
    odb_UNION_DOUBLE,
    odb_UNION_BOOL,
    odb_UNION_STRING
};

class odb_Union  : public mem_AllocationOperators
{
public:
    odb_Union( int val );
    odb_Union( float val );
    odb_Union( double val );
    odb_Union( bool val );
    odb_Union( const odb_String& val );
    odb_Union( const char* val );
    odb_Union( const odb_Union& copy );
    odb_Union& operator = ( const odb_Union& val );
    odb_UnionType type() const { return unionType; }
    int getInt() const;
    float getFloat() const;
    double getDouble() const;
    bool getBool() const;
    const odb_String& getString() const;
    ~odb_Union();

private:
    int ival;
    float fval;
    double dval;
    bool bval;
    odb_String sval;
    odb_UnionType unionType;
};

#endif
