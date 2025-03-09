//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ODB_FIELDVALUEDATA_H
#define ODB_FIELDVALUEDATA_H

#include <assert.h>
// Begin local includes
#include <mem_AllocationOperators.h>
#include <omi_types.h>
#include <odb_Types.h>
#include <odb_Enum.h>
// End local includes

//
// Forward declarations
//

class odb_FieldValueData : public mem_AllocationOperators
{
    friend odb_FieldValueData operator + ( const odb_FieldValueData &,
                                           float );
    friend odb_FieldValueData operator - ( const odb_FieldValueData &,
                                           float );
    friend odb_FieldValueData operator / ( const odb_FieldValueData &,
                                           float );
    friend odb_FieldValueData operator * ( const odb_FieldValueData &,
                                           float );
    friend odb_FieldValueData operator + ( float ,
                                           const odb_FieldValueData & );
    friend odb_FieldValueData operator - ( float ,
                                           const odb_FieldValueData & );
    friend odb_FieldValueData operator / ( float ,
                                           const odb_FieldValueData & );
    friend odb_FieldValueData operator * ( float ,
                                           const odb_FieldValueData & );
    friend odb_FieldValueData operator + ( const odb_FieldValueData &,
                                           const odb_FieldValueData& );
    friend odb_FieldValueData operator - ( const odb_FieldValueData &,
                                           const odb_FieldValueData& );
    friend odb_FieldValueData operator / ( const odb_FieldValueData &,
                                           const odb_FieldValueData& );
    friend odb_FieldValueData operator * ( const odb_FieldValueData &,
                                           const odb_FieldValueData& );

public:
    odb_FieldValueData();
    odb_FieldValueData( int width, odb_Enum::odb_PrecisionEnum precision );
    odb_FieldValueData( const float* data,
                        int width );
    odb_FieldValueData( const double* data,
                        int width );
    odb_FieldValueData( const odb_SequenceFloat& sf );
    odb_FieldValueData( const odb_SequenceDouble& sd );

    odb_FieldValueData( const odb_FieldValueData& copy );
    odb_FieldValueData& operator = ( const odb_FieldValueData& copy );
    ~odb_FieldValueData();

    //
    bool operator == ( const odb_FieldValueData& rhs ) const;
    bool operator != ( const odb_FieldValueData& rhs ) const;

    odb_Enum::odb_PrecisionEnum precision() const { return (Data ? odb_Enum::SINGLE_PRECISION : odb_Enum::DOUBLE_PRECISION); }

    // 
    void SetData( const odb_SequenceFloat& data);
    void SetData( const odb_SequenceDouble& data);
    void SetData( const float* data);
    void SetData( const double* data);
    void SetData( const float* data, const int& width );
    void SetData( const double* data, const int& width );
    void SetConjugateData( const float* data);
    void SetConjugateData( const float& data);
    void SetConjugateData( const double* data);
    void SetConjugateData( const double& data);
    void SetData( const float& data);
    void SetData( const double& data);
    void BorrowData( const float* data, const int& width );
    void BorrowData( const double* data, const int& width );
    void BorrowData( const float* data );
    void BorrowData( const double* data );
    void BorrowConjugateData( const float* data);
    void BorrowConjugateData( const double* data);

    // data
    odb_SequenceFloat data() const { return odb_SequenceFloat( ConstGet(), DataWidth ); };
    odb_SequenceFloat conjugateData() const {
      return ConjugateData ? odb_SequenceFloat( ConstGet2(), DataWidth ) : odb_SequenceFloat();
    };
    odb_SequenceDouble dataDouble() const { return odb_SequenceDouble( ConstGetDouble(), DataWidth ); };
    odb_SequenceDouble conjugateDataDouble() const {
      return ConjugateDataDouble ? odb_SequenceDouble( ConstGetDouble2(), DataWidth ) : odb_SequenceDouble();
    };
    float* Get() { return Data; };
    float* Get2() { return ConjugateData; };
    const float* ConstGet() const { return Data; };
    const float* ConstGet2() const { return ConjugateData; };
    double* GetDouble() { return DataDouble; };
    double* GetDouble2() { return ConjugateDataDouble; };
    const double* ConstGetDouble() const { return DataDouble; };
    const double* ConstGetDouble2() const { return ConjugateDataDouble; };
    float* Get( const int& column ) { assert( column < DataWidth); assert(Data); return &Data[column]; };
    float ConstGet( const int& column ) const { assert( column < DataWidth); assert(Data); return Data[column]; };
    double* GetDouble( const int& column ) { assert( column < DataWidth); assert(DataDouble); return &DataDouble[column]; };
    double ConstGetDouble( const int& column ) const { assert( column < DataWidth); assert(DataDouble); return DataDouble[column]; };

    int width() const {return DataWidth; };

    // math 
    odb_FieldValueData opPower( float pow ) const;
    odb_FieldValueData opPower( const odb_FieldValueData& pow ) const;

private:
    float*                      Data;
    float*                      ConjugateData;
    double*                     DataDouble;
    double*                     ConjugateDataDouble;
    int                         DataWidth;
    bool                        Borrowed;

    void ClearMem();
    void AllocMem( const uint width, odb_Enum::odb_PrecisionEnum prec );
};

#endif // ODB_FIELDVALUEDATA_H

