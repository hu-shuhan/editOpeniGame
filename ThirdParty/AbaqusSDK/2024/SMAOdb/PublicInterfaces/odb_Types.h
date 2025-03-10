//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_Types_h
#define odb_Types_h

// Begin local includes
#include <odb_String.h>
#include <odb_Sequence.h>
#include <cow_ListInt.h>
#include <cow_ListListInt.h>
#include <cow_ListString.h>
#include <cow_ListFloat.h>
#include <cow_ListDouble.h>
#include <cow_ListListFloat.h>
#include <cow_ListListDouble.h>

class odb_SequenceFloat;
class odb_SequenceSequenceFloat;
class odb_SequenceDouble;
class odb_SequenceSequenceDouble;

ODB_SEQUENCE_DECL(int,cow_ListInt,odb_SequenceInt);

ODB_SEQUENCE_DECL(odb_SequenceInt,cow_ListListInt,odb_SequenceSequenceInt);

ODB_SEQUENCE_DECL(odb_String,cow_ListString,odb_SequenceString);

ODB_SEQUENCE_DECL(float,cow_ListFloat,odb_SequenceFloatBase);

ODB_SEQUENCE_DECL(double,cow_ListDouble,odb_SequenceDoubleBase);

ODB_SEQUENCE_DECL(odb_SequenceFloat,cow_ListListFloat,odb_SequenceSequenceFloatBase);

ODB_SEQUENCE_DECL(odb_SequenceDouble,cow_ListListDouble,odb_SequenceSequenceDoubleBase);

COW_LIST_DECL(cow_ListListFloat,odb_1DArrayListListListFloat);
ODB_SEQUENCE_DECL(odb_SequenceSequenceFloat,odb_1DArrayListListListFloat,odb_SequenceSequenceSequenceFloat);

COW_LIST_DECL(cow_ListListDouble,odb_1DArrayListListListDouble);
ODB_SEQUENCE_DECL(odb_SequenceSequenceDouble,odb_1DArrayListListListDouble,odb_SequenceSequenceSequenceDouble);

// For command backward compatibility, we need a constructor for odb_SequenceDouble
// and odb_SequenceSequenceDouble that take their float counterparts as 
// arguments for automatic conversion.  That is why we need these classes

class odb_SequenceDouble : public odb_SequenceDoubleBase
{
public:
    odb_SequenceDouble();
    odb_SequenceDouble(const cow_ListDouble&);
    odb_SequenceDouble(const double * array, int arrayLen);
    // This constructor estimates the array with have 'length' items.
    odb_SequenceDouble(int size);
    odb_SequenceDouble(const odb_SequenceFloat&);
    odb_SequenceDouble slice( int first, int last ) const;
};

class odb_SequenceSequenceDouble : public odb_SequenceSequenceDoubleBase
{
public:
    odb_SequenceSequenceDouble();
    odb_SequenceSequenceDouble( const cow_ListListDouble& );
    odb_SequenceSequenceDouble( const odb_SequenceDouble* array, int arrayLen );
    odb_SequenceSequenceDouble( int size );
    odb_SequenceSequenceDouble( const odb_SequenceSequenceFloat& );
    odb_SequenceSequenceDouble slice( int firat, int last) const;
};

class odb_SequenceFloat : public odb_SequenceFloatBase
{
public:
    odb_SequenceFloat();
    odb_SequenceFloat(const cow_ListFloat&);
    odb_SequenceFloat(const float * array, int arrayLen);
    // This constructor estimates the array with have 'length' items.
    odb_SequenceFloat(int size);
    odb_SequenceFloat(const odb_SequenceDouble&);
    odb_SequenceFloat slice(int first, int last) const;
};

class odb_SequenceSequenceFloat : public odb_SequenceSequenceFloatBase
{
public:
    odb_SequenceSequenceFloat();
    odb_SequenceSequenceFloat( const cow_ListListFloat& );
    odb_SequenceSequenceFloat( const odb_SequenceFloat* array, int arrayLen );
    odb_SequenceSequenceFloat( int size );
    odb_SequenceSequenceFloat( const odb_SequenceSequenceDouble& );
    odb_SequenceSequenceFloat slice( int first, int last) const;    
};

#endif // #ifndef odb_Types_h

