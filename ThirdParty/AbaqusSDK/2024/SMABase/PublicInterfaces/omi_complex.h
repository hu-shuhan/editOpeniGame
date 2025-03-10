//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef omi_complex_h
#define omi_complex_h

///////////////////////////////////////////////////////////////////////////////
//
// File Name: omi_complex.C
//
// Description:
//
//    This class defines a simple class for complex numbers.  Currently this
//    class serves as a data storage class and, therefore, does not provide
//    methods/operators for any arithmetic operations.
//


//
// Includes
//

// Begin local includes
#include <mem_AllocationOperators.h>



//
// Class definition
//

class omi_complex  : public mem_AllocationOperators
{
  public:

    enum Form { CARTESIAN, POLAR };

    omi_complex() : form( CARTESIAN ), comp1( 0.0 ), comp2( 0.0 ) {}
    omi_complex( double c1, double c2 = 0.0, Form f = CARTESIAN )
	: form( f ), comp1( c1 ), comp2( c2 ) {}
    ~omi_complex() {}

    bool operator==( const omi_complex & ) const;
    bool operator!=( const omi_complex &object ) const
    { return ( !operator==( object ) ); }

    double Real() const
    { return ( ( form == CARTESIAN ) ? comp1 : ConvertedReal() ); }
    double Imag() const
    { return ( ( form == CARTESIAN ) ? comp2 : ConvertedImag() ); }

    double Modulus() const
    { return ( ( form == POLAR ) ? comp1 : ConvertedModulus() ); }
    double Argument() const
    { return ( ( form == POLAR ) ? comp2 : ConvertedArgument() ); }

    Form DataForm() const  { return ( form ); }

  protected:

    double ConvertedReal() const;
    double ConvertedImag() const;

    double ConvertedModulus() const;
    double ConvertedArgument() const;

  private:

    Form   form;			// data form
    double comp1;			// a in a+bi or r in re^thetai
    double comp2;			// b in a+bi or theta in re^thetai

};


#endif  // omi_complex_h
