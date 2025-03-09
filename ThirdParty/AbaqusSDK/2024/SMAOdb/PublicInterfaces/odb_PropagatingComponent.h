//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_PropagatingComponent_h
#define odb_PropagatingComponent_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <omi_complex.h>


// Forward declarations
class ddb_ComponentArg;


enum odb_ComponentType
{
    odb_UNSET,
    odb_DOUBLE,
    odb_COMPLEX,
    odb_FREED
};

class odb_PropagatingComponent : public mem_AllocationOperators
{
public:

    odb_PropagatingComponent(odb_ComponentType type = odb_UNSET);
    odb_PropagatingComponent( double val );
    odb_PropagatingComponent( const omi_complex& val );

    ~odb_PropagatingComponent();

    void unset();
    void free();
    void setValue( double val );
    void setComplexValue( const omi_complex& cval );

    odb_ComponentType type() const { return t; }
    double value() const { return val; }
    const omi_complex& complexValue() const { return compVal; }

    const odb_PropagatingComponent& operator=(double val);
    const odb_PropagatingComponent& operator=(const omi_complex& cval);

    ddb_ComponentArg ddbValue() const;

private:
    odb_ComponentType t;
    double val;
    omi_complex compVal;
};

#endif

