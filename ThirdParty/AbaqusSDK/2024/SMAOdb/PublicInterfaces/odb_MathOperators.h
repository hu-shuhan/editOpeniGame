//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_MathOperators_h
#define odb_MathOperators_h

// Begin local includes
#include <odb_Enum.h>
#include <odb_FieldOutput.h>
#include <odb_SequenceFieldOutput.h>
#include <odb_FieldValue.h>
#include <odb_HistoryOutput.h>
// End local includes

// Forward declarations

// fieldValues
odb_FieldValue operator + ( const odb_FieldValue &,  const odb_FieldValue & );
odb_FieldValue operator - ( const odb_FieldValue &,  const odb_FieldValue & );
odb_FieldValue operator * ( const odb_FieldValue &, const odb_FieldValue &  );
odb_FieldValue operator / ( const odb_FieldValue &, const odb_FieldValue &  );

odb_FieldValue operator + ( const odb_FieldValue &, float );
odb_FieldValue operator - ( const odb_FieldValue &, float );
odb_FieldValue operator * ( const odb_FieldValue &, float  );
odb_FieldValue operator / ( const odb_FieldValue &, float  );
odb_FieldValue operator + ( float, const odb_FieldValue & );
odb_FieldValue operator - ( float, const odb_FieldValue & );
odb_FieldValue operator * ( float, const odb_FieldValue &  );
odb_FieldValue operator / ( float, const odb_FieldValue &  );

odb_FieldValue abs(const odb_FieldValue & ); 

odb_FieldValue cos(const odb_FieldValue & );
odb_FieldValue sin(const odb_FieldValue & );
odb_FieldValue tan(const odb_FieldValue & );

odb_FieldValue acos(const odb_FieldValue & );
odb_FieldValue asin(const odb_FieldValue & );
odb_FieldValue atan(const odb_FieldValue & );

odb_FieldValue degreeToRadian(const odb_FieldValue & );
odb_FieldValue radianToDegree(const odb_FieldValue & );

odb_FieldValue power(const odb_FieldValue &, float );

odb_FieldValue exp(const odb_FieldValue & );
odb_FieldValue exp10(const odb_FieldValue & );
odb_FieldValue log(const odb_FieldValue & );
odb_FieldValue log10(const odb_FieldValue & );
odb_FieldValue sqrt(const odb_FieldValue & );


// fields
odb_FieldOutput operator + (const odb_FieldOutput &, const odb_FieldOutput & );
odb_FieldOutput operator - (const odb_FieldOutput &, const odb_FieldOutput & );
odb_FieldOutput operator * (const odb_FieldOutput &, const odb_FieldOutput &  );
odb_FieldOutput operator / (const odb_FieldOutput &, const odb_FieldOutput &  );

odb_FieldOutput operator + (const odb_FieldOutput &, float  );
odb_FieldOutput operator - (const odb_FieldOutput &, float  );
odb_FieldOutput operator * (const odb_FieldOutput &, float  );
odb_FieldOutput operator / (const odb_FieldOutput &, float  );
odb_FieldOutput operator + (float, const odb_FieldOutput & );
odb_FieldOutput operator - (float, const odb_FieldOutput & );
odb_FieldOutput operator * (float, const odb_FieldOutput &  );
odb_FieldOutput operator / (float, const odb_FieldOutput &  );

odb_FieldOutput abs(const odb_FieldOutput & ); 

odb_FieldOutput complexMagnitude(const odb_FieldOutput & ); 
odb_FieldOutput complexPhase(const odb_FieldOutput & ); 
odb_FieldOutput complexReal(const odb_FieldOutput & ); 
odb_FieldOutput complexImag(const odb_FieldOutput & ); 

odb_FieldOutput cos(const odb_FieldOutput & );
odb_FieldOutput sin(const odb_FieldOutput & );
odb_FieldOutput tan(const odb_FieldOutput & );

odb_FieldOutput acos(const odb_FieldOutput & );
odb_FieldOutput asin(const odb_FieldOutput & );
odb_FieldOutput atan(const odb_FieldOutput & );

odb_FieldOutput degreeToRadian(const odb_FieldOutput & );
odb_FieldOutput radianToDegree(const odb_FieldOutput & );

odb_FieldOutput power(const odb_FieldOutput &, float );

odb_FieldOutput exp(const odb_FieldOutput & );
odb_FieldOutput exp10(const odb_FieldOutput & );
odb_FieldOutput log(const odb_FieldOutput & );
odb_FieldOutput log10(const odb_FieldOutput & );
odb_FieldOutput sqrt(const odb_FieldOutput & );

odb_FieldOutput transpose(const odb_FieldOutput & ); // matrix transposition

odb_SequenceFieldOutput maxEnvelope(const odb_SequenceFieldOutput & );
odb_SequenceFieldOutput minEnvelope(const odb_SequenceFieldOutput & );
odb_SequenceFieldOutput maxEnvelope(const odb_SequenceFieldOutput & ,
                                    odb_Enum::odb_InvariantEnum i);
odb_SequenceFieldOutput minEnvelope(const odb_SequenceFieldOutput & ,
                                    odb_Enum::odb_InvariantEnum i);
odb_SequenceFieldOutput maxEnvelope(const odb_SequenceFieldOutput & ,
                                    const odb_String& compLabel);
odb_SequenceFieldOutput minEnvelope(const odb_SequenceFieldOutput & ,
                                    const odb_String& compLabel);

// history
odb_HistoryOutput operator + ( const odb_HistoryOutput &,
                               const odb_HistoryOutput & );
odb_HistoryOutput operator - ( const odb_HistoryOutput &,
                               const odb_HistoryOutput & );
odb_HistoryOutput operator * ( const odb_HistoryOutput &,
                               const odb_HistoryOutput &  );
odb_HistoryOutput operator / ( const odb_HistoryOutput &,
                               const odb_HistoryOutput &  );

odb_HistoryOutput operator + (const odb_HistoryOutput &, float  );
odb_HistoryOutput operator - (const odb_HistoryOutput &, float  );
odb_HistoryOutput operator * (const odb_HistoryOutput &, float  );
odb_HistoryOutput operator / (const odb_HistoryOutput &, float  );
odb_HistoryOutput operator + (float, const odb_HistoryOutput & );
odb_HistoryOutput operator - (float, const odb_HistoryOutput & );
odb_HistoryOutput operator * (float, const odb_HistoryOutput &  );
odb_HistoryOutput operator / (float, const odb_HistoryOutput &  );

odb_HistoryOutput abs(const odb_HistoryOutput & ); 

odb_HistoryOutput complexMagnitude(const odb_HistoryOutput & ); 
odb_HistoryOutput complexPhase(const odb_HistoryOutput & ); 
odb_HistoryOutput complexReal(const odb_HistoryOutput & ); 
odb_HistoryOutput complexImag(const odb_HistoryOutput & ); 

odb_HistoryOutput cos(const odb_HistoryOutput & );
odb_HistoryOutput sin(const odb_HistoryOutput & );
odb_HistoryOutput tan(const odb_HistoryOutput & );

odb_HistoryOutput acos(const odb_HistoryOutput & );
odb_HistoryOutput asin(const odb_HistoryOutput & );
odb_HistoryOutput atan(const odb_HistoryOutput & );

odb_HistoryOutput degreeToRadian(const odb_HistoryOutput & );
odb_HistoryOutput radianToDegree(const odb_HistoryOutput & );

odb_HistoryOutput power(const odb_HistoryOutput &, float );

odb_HistoryOutput exp(const odb_HistoryOutput & );
odb_HistoryOutput exp10(const odb_HistoryOutput & );
odb_HistoryOutput log(const odb_HistoryOutput & );
odb_HistoryOutput log10(const odb_HistoryOutput & );
odb_HistoryOutput sqrt(const odb_HistoryOutput & );

#endif // odb_MathOperators_h
