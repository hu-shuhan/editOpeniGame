//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
 * @CAA2Level L0
 * @CAA2Usage U0
 */
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_WipReporter_h
#define odb_WipReporter_h

// Forward declarations

// Begin local includes
#include <wip_WipReporter.h>
#include <SMABasStringMode.h>

class aio_ostream;

class odb_WipReporter : public wip_WipReporter
{
 public:
    odb_WipReporter(aio_ostream& reporter);
    virtual void Error(const atr_String&,  wip_errorLevelEnm errLevel = wip_ERROR_DEFAULT, STRMODE_DECL);
    virtual void Warning(const atr_String&, STRMODE_DECL);
    virtual void ErrorList  (const cow_ListTranslatedString&,  wip_errorLevelEnm errLevel = wip_ERROR_DEFAULT, STRMODE_DECL);
    virtual void WarningList(const cow_ListTranslatedString&, STRMODE_DECL);



    virtual void Verbose(const atr_String&, STRMODE_DECL);
    virtual void Milestone(const atr_String&, STRMODE_DECL);
    virtual void Landmark(const atr_String&, STRMODE_DECL);
    virtual void Milestone(const atr_String&, const atr_String&, int, int, STRMODE_DECL);
    virtual void Milestone(const atr_String&, int, STRMODE_DECL);
    virtual void Print(const atr_String&, STRMODE_DECL);
    virtual void Display(const atr_String&, STRMODE_DECL);

    virtual int Interrupted();
    virtual void CommandDone();
    virtual void InterruptedThrow();
    void GenerateWipMessages(bool  value = false );

 private:
    // Static data member for turning on/off generation of
    // WIP messages for all objects. Used in compFilOdb
    // program. Note that it really only needs to be used
    // on informational WIP messages which will be journaled
    // (e.g. Landmarks, Verbose)
    static bool  generateWipMessages;

    aio_ostream& reporter;
};

#endif //ifdef odb_WipReporter_h



