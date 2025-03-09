//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2014
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef SMABasAtrStringMode_h
#define SMABasAtrStringMode_h

#include <atr_String.h>
#include <SMABasStringMode.h>

namespace SMABasAtrStringMode {
    inline atr_String
    Utf8ToModal(const atr_String& exp, STRMODE_DEF) {
        if (strMode.IsLocale()) {
            return exp.UTF8ToLocale();
        } else {
            return exp;
        }
    }

    inline atr_String
    LocaleToModal(const atr_String& exp, STRMODE_DEF) {
        if (strMode.IsLocale()) {
            return exp;
        } else {
            return exp.LocaleToUTF8();
        }
    }

    inline atr_String
    ModalToUTF8(const atr_String& exp, STRMODE_DEF) {
        if (strMode.IsLocale()) {
            return exp.LocaleToUTF8();
        } else {
            return exp;
        }
    }

    inline atr_String
    ModalToLocale(const atr_String& exp, STRMODE_DEF) {
        if (strMode.IsLocale()) {
            return exp;
        } else {
            return exp.UTF8ToLocale();
        }
    }
};

// Some macros to simplify modal translations.
#define ATR_UTF82MODAL(exp) SMABasAtrStringMode::Utf8ToModal(exp, strMode)
#define ATR_LOCALE2MODAL(exp) SMABasAtrStringMode::LocaleToModal(exp, strMode)
#define ATR_MODAL2LOCALE(exp) SMABasAtrStringMode::ModalToLocale(exp, strMode)
#define ATR_MODAL2UTF8(exp) SMABasAtrStringMode::ModalToUTF8(exp, strMode)

#endif
