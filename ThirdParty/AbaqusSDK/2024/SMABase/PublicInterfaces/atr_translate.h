//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef atr_translate_h
#define atr_translate_h

// This function provides an interface to translate a character string.
//#include <stdarg.h>
// Begin local includes
#include <mem_AllocationOperators.h>
#include <mem_AllocationOperatorsTiny.h>
#include <mem_C_Allocation.h>
#include <omi_types.h>
#include <atr_String.h>
#include <SMABasStringMode.h>

class atr_Fmt  : public mem_AllocationOperators
{
  friend class atr_translate;
public:
  enum DataSize { DataSizeDefault, DataSizeShort, DataSizeLong, DataSizeHuge };

  atr_Fmt(char const* fmt);
  ~atr_Fmt();

  bool Ok() const { return m_Fmt != NULL; };

  char Letter();
  char const* Format(DataSize dataSize = DataSizeDefault, char letterOverride = 0);
  char const* FormatNoWidth(DataSize dataSize = DataSizeDefault, char letterOverride = 0);

private:
  char const* m_Fmt;
  char m_Letter;
  char* m_OutFmt;

  atr_Fmt(atr_Fmt const&);
  atr_Fmt& operator=(atr_Fmt const&);
};

class atr_translate  : public mem_AllocationOperators
{
public:
    atr_translate(char const* pStr, STRMODE_DECL);
  ~atr_translate();

  char const* Format(int pos) const;
  void AddSubstitution(int pos, char const* subst, char const* fmt = 0);

  static void FixExponent(char* buffer);

  atr_String Result(STRMODE_DECL) const;

private:
  struct StrArgs : public mem_AllocationOperatorsTiny
  {
    StrArgs() : sFormat(0), sValue(0) {};
    ~StrArgs() { mem_Free(sFormat); mem_Free(sValue); };

    int nPos;
    char* sFormat;
    char* sValue;
  };

  static int compare( const void *arg1, const void *arg2 );
  void ReportIllegalPosition(int pos) const;

  char const* m_OriginalFormat;
  atr_String m_originalFormatUTF8;

  int nCount;
  StrArgs* aArgs;
  StrArgs** ppSortArgs;
  char* psOutput;

  atr_translate(atr_translate const&);
  atr_translate& operator=(atr_translate const&);
};

atr_String atr_raw(const char *pStr, STRMODE_DECL);

void atr_SubstituteFormat(atr_translate& format, int position, int data, STRMODE_DECL);
void atr_SubstituteFormat(atr_translate& format, int position, uint data, STRMODE_DECL);
void atr_SubstituteFormat(atr_translate& format, int position, int64_t data, STRMODE_DECL);
void atr_SubstituteFormat(atr_translate& format, int position, uint64_t data, STRMODE_DECL);
void atr_SubstituteFormat(atr_translate& format, int position, float data, STRMODE_DECL);
void atr_SubstituteFormat(atr_translate& format, int position, double data, STRMODE_DECL);
void atr_SubstituteFormat(atr_translate& format, int position, char const* data, STRMODE_DECL);
void atr_SubstituteFormat(atr_translate& format, int position, void const* data, STRMODE_DECL);
void atr_SubstituteFormat(atr_translate& format, int position, atr_StringBase const& data, STRMODE_DECL);

inline atr_String atr(char const* format, STRMODE_DECL)
{
    atr_translate fmt(format, strMode);
    return fmt.Result(strMode);
}

template <typename T1>
inline atr_String atr(char const* format, T1 data1, STRMODE_DECL)
{
    atr_translate fmt(format, strMode);
    atr_SubstituteFormat(fmt, 1, data1, strMode);
    return fmt.Result(strMode);
}

template <typename T1, typename T2>
    inline atr_String atr(char const* format, T1 data1, T2 data2, STRMODE_DECL)
{
    atr_translate fmt(format, strMode);
    atr_SubstituteFormat(fmt, 1, data1, strMode);
    atr_SubstituteFormat(fmt, 2, data2, strMode);
    return fmt.Result(strMode);
}

template <typename T1, typename T2, typename T3>
    inline atr_String atr(char const* format, T1 data1, T2 data2, T3 data3, STRMODE_DECL)
{
    atr_translate fmt(format, strMode);
    atr_SubstituteFormat(fmt, 1, data1, strMode);
    atr_SubstituteFormat(fmt, 2, data2, strMode);
    atr_SubstituteFormat(fmt, 3, data3, strMode);
    return fmt.Result(strMode);
}

template <typename T1, typename T2, typename T3, typename T4>
inline atr_String atr(char const* format,
                      T1 data1, T2 data2, T3 data3, T4 data4, STRMODE_DECL)
{
    atr_translate fmt(format, strMode);
    atr_SubstituteFormat(fmt, 1, data1, strMode);
    atr_SubstituteFormat(fmt, 2, data2, strMode);
    atr_SubstituteFormat(fmt, 3, data3, strMode);
    atr_SubstituteFormat(fmt, 4, data4, strMode);
    return fmt.Result(strMode);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5>
inline atr_String atr(char const* format,
                      T1 data1, T2 data2, T3 data3, T4 data4, T5 data5, STRMODE_DECL)
{
    atr_translate fmt(format, strMode);
    atr_SubstituteFormat(fmt, 1, data1, strMode);
    atr_SubstituteFormat(fmt, 2, data2, strMode);
    atr_SubstituteFormat(fmt, 3, data3, strMode);
    atr_SubstituteFormat(fmt, 4, data4, strMode);
    atr_SubstituteFormat(fmt, 5, data5, strMode);
    return fmt.Result(strMode);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6>
inline atr_String atr(char const* format,
                      T1 data1, T2 data2, T3 data3, T4 data4, T5 data5,
                      T6 data6, STRMODE_DECL)
{
    atr_translate fmt(format, strMode);
    atr_SubstituteFormat(fmt, 1, data1, strMode);
    atr_SubstituteFormat(fmt, 2, data2, strMode);
    atr_SubstituteFormat(fmt, 3, data3, strMode);
    atr_SubstituteFormat(fmt, 4, data4, strMode);
    atr_SubstituteFormat(fmt, 5, data5, strMode);
    atr_SubstituteFormat(fmt, 6, data6, strMode);
    return fmt.Result(strMode);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7>
inline atr_String atr(char const* format,
                      T1 data1, T2 data2, T3 data3, T4 data4, T5 data5,
                      T6 data6, T7 data7, STRMODE_DECL)
{
    atr_translate fmt(format, strMode);
    atr_SubstituteFormat(fmt, 1, data1, strMode);
    atr_SubstituteFormat(fmt, 2, data2, strMode);
    atr_SubstituteFormat(fmt, 3, data3, strMode);
    atr_SubstituteFormat(fmt, 4, data4, strMode);
    atr_SubstituteFormat(fmt, 5, data5, strMode);
    atr_SubstituteFormat(fmt, 6, data6, strMode);
    atr_SubstituteFormat(fmt, 7, data7, strMode);
    return fmt.Result(strMode);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8>
inline atr_String atr(char const* format,
                      T1 data1, T2 data2, T3 data3, T4 data4, T5 data5,
                      T6 data6, T7 data7, T8 data8, STRMODE_DECL)
{
    atr_translate fmt(format, strMode);
    atr_SubstituteFormat(fmt, 1, data1, strMode);
    atr_SubstituteFormat(fmt, 2, data2, strMode);
    atr_SubstituteFormat(fmt, 3, data3, strMode);
    atr_SubstituteFormat(fmt, 4, data4, strMode);
    atr_SubstituteFormat(fmt, 5, data5, strMode);
    atr_SubstituteFormat(fmt, 6, data6, strMode);
    atr_SubstituteFormat(fmt, 7, data7, strMode);
    atr_SubstituteFormat(fmt, 8, data8, strMode);
    return fmt.Result(strMode);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9>
inline atr_String atr(char const* format,
                      T1 data1, T2 data2, T3 data3, T4 data4, T5 data5,
                      T6 data6, T7 data7, T8 data8, T9 data9, STRMODE_DECL)
{
    atr_translate fmt(format, strMode);
    atr_SubstituteFormat(fmt, 1, data1, strMode);
    atr_SubstituteFormat(fmt, 2, data2, strMode);
    atr_SubstituteFormat(fmt, 3, data3, strMode);
    atr_SubstituteFormat(fmt, 4, data4, strMode);
    atr_SubstituteFormat(fmt, 5, data5, strMode);
    atr_SubstituteFormat(fmt, 6, data6, strMode);
    atr_SubstituteFormat(fmt, 7, data7, strMode);
    atr_SubstituteFormat(fmt, 8, data8, strMode);
    atr_SubstituteFormat(fmt, 9, data9, strMode);
    return fmt.Result(strMode);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10>
inline atr_String atr(char const* format,
                      T1 data1, T2 data2, T3 data3, T4 data4, T5 data5,
                      T6 data6, T7 data7, T8 data8, T9 data9, T10 data10, STRMODE_DECL)
{
    atr_translate fmt(format, strMode);
    atr_SubstituteFormat(fmt, 1, data1, strMode);
    atr_SubstituteFormat(fmt, 2, data2, strMode);
    atr_SubstituteFormat(fmt, 3, data3, strMode);
    atr_SubstituteFormat(fmt, 4, data4, strMode);
    atr_SubstituteFormat(fmt, 5, data5, strMode);
    atr_SubstituteFormat(fmt, 6, data6, strMode);
    atr_SubstituteFormat(fmt, 7, data7, strMode);
    atr_SubstituteFormat(fmt, 8, data8, strMode);
    atr_SubstituteFormat(fmt, 9, data9, strMode);
    atr_SubstituteFormat(fmt, 10, data10, strMode);
    return fmt.Result(strMode);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
          typename T11>
inline atr_String atr(char const* format,
                      T1 data1, T2 data2, T3 data3, T4 data4, T5 data5,
                      T6 data6, T7 data7, T8 data8, T9 data9, T10 data10,
                      T11 data11, STRMODE_DECL)
{
    atr_translate fmt(format, strMode);
    atr_SubstituteFormat(fmt, 1, data1, strMode);
    atr_SubstituteFormat(fmt, 2, data2, strMode);
    atr_SubstituteFormat(fmt, 3, data3, strMode);
    atr_SubstituteFormat(fmt, 4, data4, strMode);
    atr_SubstituteFormat(fmt, 5, data5, strMode);
    atr_SubstituteFormat(fmt, 6, data6, strMode);
    atr_SubstituteFormat(fmt, 7, data7, strMode);
    atr_SubstituteFormat(fmt, 8, data8, strMode);
    atr_SubstituteFormat(fmt, 9, data9, strMode);
    atr_SubstituteFormat(fmt, 10, data10, strMode);
    atr_SubstituteFormat(fmt, 11, data11, strMode);
    return fmt.Result(strMode);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
          typename T11, typename T12>
inline atr_String atr(char const* format,
                      T1 data1, T2 data2, T3 data3, T4 data4, T5 data5,
                      T6 data6, T7 data7, T8 data8, T9 data9, T10 data10,
                      T11 data11, T12 data12, STRMODE_DECL)
{
    atr_translate fmt(format, strMode);
    atr_SubstituteFormat(fmt, 1, data1, strMode);
    atr_SubstituteFormat(fmt, 2, data2, strMode);
    atr_SubstituteFormat(fmt, 3, data3, strMode);
    atr_SubstituteFormat(fmt, 4, data4, strMode);
    atr_SubstituteFormat(fmt, 5, data5, strMode);
    atr_SubstituteFormat(fmt, 6, data6, strMode);
    atr_SubstituteFormat(fmt, 7, data7, strMode);
    atr_SubstituteFormat(fmt, 8, data8, strMode);
    atr_SubstituteFormat(fmt, 9, data9, strMode);
    atr_SubstituteFormat(fmt, 10, data10, strMode);
    atr_SubstituteFormat(fmt, 11, data11, strMode);
    atr_SubstituteFormat(fmt, 12, data12, strMode);
    return fmt.Result(strMode);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
          typename T11, typename T12, typename T13>
inline atr_String atr(char const* format,
                      T1 data1, T2 data2, T3 data3, T4 data4, T5 data5,
                      T6 data6, T7 data7, T8 data8, T9 data9, T10 data10,
                      T11 data11, T12 data12, T13 data13, STRMODE_DECL)
{
    atr_translate fmt(format, strMode);
    atr_SubstituteFormat(fmt, 1, data1, strMode);
    atr_SubstituteFormat(fmt, 2, data2, strMode);
    atr_SubstituteFormat(fmt, 3, data3, strMode);
    atr_SubstituteFormat(fmt, 4, data4, strMode);
    atr_SubstituteFormat(fmt, 5, data5, strMode);
    atr_SubstituteFormat(fmt, 6, data6, strMode);
    atr_SubstituteFormat(fmt, 7, data7, strMode);
    atr_SubstituteFormat(fmt, 8, data8, strMode);
    atr_SubstituteFormat(fmt, 9, data9, strMode);
    atr_SubstituteFormat(fmt, 10, data10, strMode);
    atr_SubstituteFormat(fmt, 11, data11, strMode);
    atr_SubstituteFormat(fmt, 12, data12, strMode);
    atr_SubstituteFormat(fmt, 13, data13, strMode);
    return fmt.Result(strMode);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
          typename T11, typename T12, typename T13, typename T14>
inline atr_String atr(char const* format,
                      T1 data1, T2 data2, T3 data3, T4 data4, T5 data5,
                      T6 data6, T7 data7, T8 data8, T9 data9, T10 data10,
                      T11 data11, T12 data12, T13 data13, T14 data14, STRMODE_DECL)
{
    atr_translate fmt(format, strMode);
    atr_SubstituteFormat(fmt, 1, data1, strMode);
    atr_SubstituteFormat(fmt, 2, data2, strMode);
    atr_SubstituteFormat(fmt, 3, data3, strMode);
    atr_SubstituteFormat(fmt, 4, data4, strMode);
    atr_SubstituteFormat(fmt, 5, data5, strMode);
    atr_SubstituteFormat(fmt, 6, data6, strMode);
    atr_SubstituteFormat(fmt, 7, data7, strMode);
    atr_SubstituteFormat(fmt, 8, data8, strMode);
    atr_SubstituteFormat(fmt, 9, data9, strMode);
    atr_SubstituteFormat(fmt, 10, data10, strMode);
    atr_SubstituteFormat(fmt, 11, data11, strMode);
    atr_SubstituteFormat(fmt, 12, data12, strMode);
    atr_SubstituteFormat(fmt, 13, data13, strMode);
    atr_SubstituteFormat(fmt, 14, data14, strMode);
    return fmt.Result(strMode);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
          typename T11, typename T12, typename T13, typename T14, typename T15>
inline atr_String atr(char const* format,
                      T1 data1, T2 data2, T3 data3, T4 data4, T5 data5,
                      T6 data6, T7 data7, T8 data8, T9 data9, T10 data10,
                      T11 data11, T12 data12, T13 data13, T14 data14, T15 data15, STRMODE_DECL)
{
    atr_translate fmt(format, strMode);
    atr_SubstituteFormat(fmt, 1, data1, strMode);
    atr_SubstituteFormat(fmt, 2, data2, strMode);
    atr_SubstituteFormat(fmt, 3, data3, strMode);
    atr_SubstituteFormat(fmt, 4, data4, strMode);
    atr_SubstituteFormat(fmt, 5, data5, strMode);
    atr_SubstituteFormat(fmt, 6, data6, strMode);
    atr_SubstituteFormat(fmt, 7, data7, strMode);
    atr_SubstituteFormat(fmt, 8, data8, strMode);
    atr_SubstituteFormat(fmt, 9, data9, strMode);
    atr_SubstituteFormat(fmt, 10, data10, strMode);
    atr_SubstituteFormat(fmt, 11, data11, strMode);
    atr_SubstituteFormat(fmt, 12, data12, strMode);
    atr_SubstituteFormat(fmt, 13, data13, strMode);
    atr_SubstituteFormat(fmt, 14, data14, strMode);
    atr_SubstituteFormat(fmt, 15, data15, strMode);
    return fmt.Result(strMode);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
          typename T11, typename T12, typename T13, typename T14, typename T15,
          typename T16>
inline atr_String atr(char const* format,
                      T1 data1, T2 data2, T3 data3, T4 data4, T5 data5,
                      T6 data6, T7 data7, T8 data8, T9 data9, T10 data10,
                      T11 data11, T12 data12, T13 data13, T14 data14, T15 data15,
                      T16 data16, STRMODE_DECL)
{
    atr_translate fmt(format, strMode);
    atr_SubstituteFormat(fmt, 1, data1, strMode);
    atr_SubstituteFormat(fmt, 2, data2, strMode);
    atr_SubstituteFormat(fmt, 3, data3, strMode);
    atr_SubstituteFormat(fmt, 4, data4, strMode);
    atr_SubstituteFormat(fmt, 5, data5, strMode);
    atr_SubstituteFormat(fmt, 6, data6, strMode);
    atr_SubstituteFormat(fmt, 7, data7, strMode);
    atr_SubstituteFormat(fmt, 8, data8, strMode);
    atr_SubstituteFormat(fmt, 9, data9, strMode);
    atr_SubstituteFormat(fmt, 10, data10, strMode);
    atr_SubstituteFormat(fmt, 11, data11, strMode);
    atr_SubstituteFormat(fmt, 12, data12, strMode);
    atr_SubstituteFormat(fmt, 13, data13, strMode);
    atr_SubstituteFormat(fmt, 14, data14, strMode);
    atr_SubstituteFormat(fmt, 15, data15, strMode);
    atr_SubstituteFormat(fmt, 16, data16, strMode);
    return fmt.Result(strMode);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
          typename T11, typename T12, typename T13, typename T14, typename T15,
          typename T16, typename T17>
inline atr_String atr(char const* format,
                      T1 data1, T2 data2, T3 data3, T4 data4, T5 data5,
                      T6 data6, T7 data7, T8 data8, T9 data9, T10 data10,
                      T11 data11, T12 data12, T13 data13, T14 data14, T15 data15,
                      T16 data16, T17 data17, STRMODE_DECL)
{
    atr_translate fmt(format, strMode);
    atr_SubstituteFormat(fmt, 1, data1, strMode);
    atr_SubstituteFormat(fmt, 2, data2, strMode);
    atr_SubstituteFormat(fmt, 3, data3, strMode);
    atr_SubstituteFormat(fmt, 4, data4, strMode);
    atr_SubstituteFormat(fmt, 5, data5, strMode);
    atr_SubstituteFormat(fmt, 6, data6, strMode);
    atr_SubstituteFormat(fmt, 7, data7, strMode);
    atr_SubstituteFormat(fmt, 8, data8, strMode);
    atr_SubstituteFormat(fmt, 9, data9, strMode);
    atr_SubstituteFormat(fmt, 10, data10, strMode);
    atr_SubstituteFormat(fmt, 11, data11, strMode);
    atr_SubstituteFormat(fmt, 12, data12, strMode);
    atr_SubstituteFormat(fmt, 13, data13, strMode);
    atr_SubstituteFormat(fmt, 14, data14, strMode);
    atr_SubstituteFormat(fmt, 15, data15, strMode);
    atr_SubstituteFormat(fmt, 16, data16, strMode);
    atr_SubstituteFormat(fmt, 17, data17, strMode);
    return fmt.Result(strMode);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
          typename T11, typename T12, typename T13, typename T14, typename T15,
          typename T16, typename T17, typename T18>
inline atr_String atr(char const* format,
                      T1 data1, T2 data2, T3 data3, T4 data4, T5 data5,
                      T6 data6, T7 data7, T8 data8, T9 data9, T10 data10,
                      T11 data11, T12 data12, T13 data13, T14 data14, T15 data15,
                      T16 data16, T17 data17, T18 data18, STRMODE_DECL)
{
    atr_translate fmt(format, strMode);
    atr_SubstituteFormat(fmt, 1, data1, strMode);
    atr_SubstituteFormat(fmt, 2, data2, strMode);
    atr_SubstituteFormat(fmt, 3, data3, strMode);
    atr_SubstituteFormat(fmt, 4, data4, strMode);
    atr_SubstituteFormat(fmt, 5, data5, strMode);
    atr_SubstituteFormat(fmt, 6, data6, strMode);
    atr_SubstituteFormat(fmt, 7, data7, strMode);
    atr_SubstituteFormat(fmt, 8, data8, strMode);
    atr_SubstituteFormat(fmt, 9, data9, strMode);
    atr_SubstituteFormat(fmt, 10, data10, strMode);
    atr_SubstituteFormat(fmt, 11, data11, strMode);
    atr_SubstituteFormat(fmt, 12, data12, strMode);
    atr_SubstituteFormat(fmt, 13, data13, strMode);
    atr_SubstituteFormat(fmt, 14, data14, strMode);
    atr_SubstituteFormat(fmt, 15, data15, strMode);
    atr_SubstituteFormat(fmt, 16, data16, strMode);
    atr_SubstituteFormat(fmt, 17, data17, strMode);
    atr_SubstituteFormat(fmt, 18, data18, strMode);
    return fmt.Result(strMode);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
          typename T11, typename T12, typename T13, typename T14, typename T15,
          typename T16, typename T17, typename T18, typename T19>
inline atr_String atr(char const* format,
                      T1 data1, T2 data2, T3 data3, T4 data4, T5 data5,
                      T6 data6, T7 data7, T8 data8, T9 data9, T10 data10,
                      T11 data11, T12 data12, T13 data13, T14 data14, T15 data15,
                      T16 data16, T17 data17, T18 data18, T19 data19, STRMODE_DECL)
{
    atr_translate fmt(format, strMode);
    atr_SubstituteFormat(fmt, 1, data1, strMode);
    atr_SubstituteFormat(fmt, 2, data2, strMode);
    atr_SubstituteFormat(fmt, 3, data3, strMode);
    atr_SubstituteFormat(fmt, 4, data4, strMode);
    atr_SubstituteFormat(fmt, 5, data5, strMode);
    atr_SubstituteFormat(fmt, 6, data6, strMode);
    atr_SubstituteFormat(fmt, 7, data7, strMode);
    atr_SubstituteFormat(fmt, 8, data8, strMode);
    atr_SubstituteFormat(fmt, 9, data9, strMode);
    atr_SubstituteFormat(fmt, 10, data10, strMode);
    atr_SubstituteFormat(fmt, 11, data11, strMode);
    atr_SubstituteFormat(fmt, 12, data12, strMode);
    atr_SubstituteFormat(fmt, 13, data13, strMode);
    atr_SubstituteFormat(fmt, 14, data14, strMode);
    atr_SubstituteFormat(fmt, 15, data15, strMode);
    atr_SubstituteFormat(fmt, 16, data16, strMode);
    atr_SubstituteFormat(fmt, 17, data17, strMode);
    atr_SubstituteFormat(fmt, 18, data18, strMode);
    atr_SubstituteFormat(fmt, 19, data19, strMode);
    return fmt.Result(strMode);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
          typename T11, typename T12, typename T13, typename T14, typename T15,
          typename T16, typename T17, typename T18, typename T19, typename T20>
inline atr_String atr(char const* format,
                      T1 data1, T2 data2, T3 data3, T4 data4, T5 data5,
                      T6 data6, T7 data7, T8 data8, T9 data9, T10 data10,
                      T11 data11, T12 data12, T13 data13, T14 data14, T15 data15,
                      T16 data16, T17 data17, T18 data18, T19 data19, T20 data20, STRMODE_DECL)
{
    atr_translate fmt(format, strMode);
    atr_SubstituteFormat(fmt, 1, data1, strMode);
    atr_SubstituteFormat(fmt, 2, data2, strMode);
    atr_SubstituteFormat(fmt, 3, data3, strMode);
    atr_SubstituteFormat(fmt, 4, data4, strMode);
    atr_SubstituteFormat(fmt, 5, data5, strMode);
    atr_SubstituteFormat(fmt, 6, data6, strMode);
    atr_SubstituteFormat(fmt, 7, data7, strMode);
    atr_SubstituteFormat(fmt, 8, data8, strMode);
    atr_SubstituteFormat(fmt, 9, data9, strMode);
    atr_SubstituteFormat(fmt, 10, data10, strMode);
    atr_SubstituteFormat(fmt, 11, data11, strMode);
    atr_SubstituteFormat(fmt, 12, data12, strMode);
    atr_SubstituteFormat(fmt, 13, data13, strMode);
    atr_SubstituteFormat(fmt, 14, data14, strMode);
    atr_SubstituteFormat(fmt, 15, data15, strMode);
    atr_SubstituteFormat(fmt, 16, data16, strMode);
    atr_SubstituteFormat(fmt, 17, data17, strMode);
    atr_SubstituteFormat(fmt, 18, data18, strMode);
    atr_SubstituteFormat(fmt, 19, data19, strMode);
    atr_SubstituteFormat(fmt, 20, data20, strMode);
    return fmt.Result(strMode);
}

#endif // atr_translate_h
