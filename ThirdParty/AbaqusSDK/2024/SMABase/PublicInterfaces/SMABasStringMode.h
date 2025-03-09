//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2013
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef SMABasStringMode_h
#define SMABasStringMode_h

#if defined(_CAE_MODE_) || defined(ODB_LOCALE_STRINGS)
#   define IS_LOCALE true
#else
#   define IS_LOCALE false
#endif
#define STRMODE_DECL const SMABasStringMode strMode = SMABasStringMode(IS_LOCALE)
#define STRMODE_DEF const SMABasStringMode strMode

class SMABasStringMode {
 public:
    explicit SMABasStringMode(bool isLocale) : _val(isLocale) {};
    bool IsLocale() const {
        return _val;
    }
 private:
    // Do not support default constructor
    SMABasStringMode(void);
    bool _val;
};

#endif
