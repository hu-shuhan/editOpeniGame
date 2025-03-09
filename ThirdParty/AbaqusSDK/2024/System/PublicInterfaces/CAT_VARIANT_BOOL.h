#ifndef CAT_VARIANT_BOOL_h
#define CAT_VARIANT_BOOL_h

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */
 
#include "CATBoolean.h"

// COPYRIGHT DASSAULT SYSTEMES 2001

/**
 * This is the definition for the Automation VARIANT_BOOL type.
 *
 * Once all the implementations which use booleans have migrated to
 * CAT_VARIANT_BOOL, CAT_VARIANT_TRUE, and CAT_VARIANT_FALSE,
 * the real definition for them will be substituted.
 * typedef short CAT_VARIANT_BOOL;
 * #define CAT_VARIANT_TRUE  -1 (0xFFFF)
 * #define CAT_VARIANT_FALSE 0x0000
 * This implies that you must not affect a CAT_VARIANT_BOOL to an unsigned char (CATBoolean, …) and vice versa. 
 * Use instead BOOL_TO_VARBOOL or VARBOOL_TO_BOOL macros.
 */
 

#ifdef REMOVE_USELESS_INCLUDE
#ifndef DEFINE_CATVARIANTBOOL_AS_CLASS
#define DEFINE_CATVARIANTBOOL_AS_CLASS
#endif
#endif

#ifdef DEFINE_CATVARIANTBOOL_AS_CLASS
#pragma message (" BADREM : JBX 100610 " __FILE__ " Affect or compare a CAT_VARIANT_BOOL only with another CAT_VARIANT_BOOL")
class CAT_VARIANT_BOOL
{
public:
	
	CAT_VARIANT_BOOL() {_rd=1;};
	CAT_VARIANT_BOOL(const CAT_VARIANT_BOOL &iOther){_rd=iOther._rd;}; 
	CAT_VARIANT_BOOL(int i1, int i2, int i3){ _rd = (i1) ? (-1) : (0);};
    CAT_VARIANT_BOOL&  operator=(const CAT_VARIANT_BOOL &iOther) {_rd=iOther._rd;return *this;};
	int operator == ( const CAT_VARIANT_BOOL &iOther ) const { return ((_rd==iOther._rd) ? (1) : (0));} ;
   	int operator != ( const CAT_VARIANT_BOOL &iOther ) const { return ((_rd!=iOther._rd) ? (1) : (0));};
	
	short _rd;	
	
private:	
    operator unsigned char() const;
	operator int() const;	
	
	// Affect operator are not allowed
   	CAT_VARIANT_BOOL &operator =(const int &iOther) ;
	CAT_VARIANT_BOOL &operator =(const unsigned char &iOther) ;
	CAT_VARIANT_BOOL &operator =(const char &iOther) ;
	CAT_VARIANT_BOOL &operator =(const long &iOther) ;
	
	// Equality comparison operators not allowed with int and unsigned char and long
   	int operator == ( const unsigned char &iOther ) const ;
   	int operator != ( const unsigned char &iOther ) const ;
	int operator == ( const char &iOther ) const ;
   	int operator != ( const char &iOther ) const ;	
	int operator == ( const int &iOther ) const ;
   	int operator != ( const int &iOther ) const ;
	int operator == ( const long &iOther ) const ;
   	int operator != ( const long &iOther ) const ;
	
	
	// Ordering comparison operators not allowed
	int operator <  ( const CAT_VARIANT_BOOL &iOther ) const ;
	int operator <= ( const CAT_VARIANT_BOOL &iOther ) const ;
	int operator >= ( const CAT_VARIANT_BOOL &iOther ) const ;
	int operator >  ( const CAT_VARIANT_BOOL &iOther ) const ;
	
	int operator <  ( const unsigned char &iOther ) const ;
	int operator <= ( const unsigned char &iOther ) const ;
	int operator >= ( const unsigned char &iOther ) const ;
	int operator >  ( const unsigned char &iOther ) const ;
	
	int operator <  ( const char &iOther ) const ;
	int operator <= ( const char &iOther ) const ;
	int operator >= ( const char &iOther ) const ;
	int operator >  ( const char &iOther ) const ;
	
    int operator <  ( const int &iOther ) const ;
	int operator <= ( const int &iOther ) const ;
	int operator >= ( const int &iOther ) const ;
	int operator >  ( const int &iOther ) const ;    
};

/** 
 * Conversion from CAT_VARIANT_BOOL to CATBoolean and vice versa
 */
static const CAT_VARIANT_BOOL CAT_VARIANT_FALSE(0,0,0);
static const CAT_VARIANT_BOOL CAT_VARIANT_TRUE(1,1,1);

#define BOOL_TO_VARBOOL(x) ((x) ? (CAT_VARIANT_TRUE) : (CAT_VARIANT_FALSE) )
#define VARBOOL_TO_BOOL(x) ((x._rd) ? (TRUE) : (FALSE) )

#else

typedef short CAT_VARIANT_BOOL;

#ifndef CAT_VARIANT_TRUE
#define CAT_VARIANT_TRUE ((short)-1)
#endif 

#ifndef CAT_VARIANT_FALSE
#define CAT_VARIANT_FALSE ((short)0)
#endif 

/** 
 * Conversion from CAT_VARIANT_BOOL to CATBoolean and vice versa
 */
#define BOOL_TO_VARBOOL(x) ((x) ? (CAT_VARIANT_TRUE) : (CAT_VARIANT_FALSE) )
#define VARBOOL_TO_BOOL(x) ((x) ? (TRUE) : (FALSE) )

#endif // DEFINE_CATVARIANTBOOL_AS_CLASS

#endif // CAT_VARIANT_BOOL_h

