//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef  wip_WipReporter_h
#define  wip_WipReporter_h

///////////////////////////////////////////////////////////////////////////////
//
// File Name: wip_WipReporter.h
//
// Description: This class implements the strategy participant of the pattern
// Strategy described in GOF (pg 315).
//      

// Begin local includes
#include <mem_AllocationOperators.h>
#include <SMABasStringMode.h>

// Forward declarations

class atr_String;
template <class Item> class cow_List;
typedef cow_List<atr_String> cow_ListTranslatedString;


enum wip_errorLevelEnm {
        wip_ERROR_DEFAULT  = 0,
	wip_ERROR_CONTINUE = 1,
	wip_ERROR_FATAL    = 2
    };

enum wip_MessagePostTypeEnm {
        wip_WARNING_NO_DIALOG,
        wip_WARNING_DIALOG_POSTING,
        wip_DISMISSABLE_WARNING_DIALOG_POSTING,
        wip_ITEM_LIST_WARNING_DIALOG_POSTING,
        wip_MILESTONE,
        wip_MILESTONE_PERCENT,
        wip_MILESTONE_COUNT,
        wip_LANDMARK,
        wip_VERBOSE
    };

class omu_PrimTuple;

// Class definition - wip_WipReporter

class wip_WipReporter  : public mem_AllocationOperators
{
public:

    wip_WipReporter():
	showWarnings(true),
	showMilestones(true),
	showLandmarks(true),
	showVerboses(true),
	showDisplays(true) {}

    virtual ~wip_WipReporter();

    virtual void Error  (const atr_String&, 
                         wip_errorLevelEnm errLevel = wip_ERROR_DEFAULT,
                         STRMODE_DECL) = 0;
    virtual void Warning(const atr_String&, STRMODE_DECL) = 0;
    virtual void ErrorList  (const cow_ListTranslatedString&, 
                             wip_errorLevelEnm errLevel = wip_ERROR_DEFAULT,
                             STRMODE_DECL) = 0;
    virtual void WarningList(const cow_ListTranslatedString&, STRMODE_DECL) = 0;

    // Interface for facility messages internationalization.
    virtual void FacilityMessage(const atr_String& defaultText,
                                 int postType,
                                 int facId,
                                 int msgId,
                                 omu_PrimTuple& msgArgs, 
                                 STRMODE_DECL );

    virtual void Verbose  (const atr_String&, STRMODE_DECL) = 0;
    virtual void Milestone(const atr_String&, STRMODE_DECL) = 0;
    virtual void Landmark (const atr_String&, STRMODE_DECL) = 0;
    virtual void Print    (const atr_String&, STRMODE_DECL) = 0;
    virtual void Display  (const atr_String&, STRMODE_DECL) = 0;

    // Used by the kernel
    virtual void Milestone(const atr_String&, const atr_String&, int, int,
                           STRMODE_DECL) = 0;
    virtual void Milestone(const atr_String&, int, STRMODE_DECL) = 0;
    
    virtual int  Interrupted() = 0;
    virtual void CommandDone() = 0;
    virtual void InterruptedThrow() = 0;
    
    // methods for enabling/disabling messages by type.
    void SetShowWarningsState(bool val)  { showWarnings  = val; }
    void SetShowMilestonesState(bool val) { showMilestones = val; }
    void SetShowLandmarksState(bool val) { showLandmarks = val; }
    void SetShowVerbosesState(bool val)  { showVerboses  = val; }
    void SetShowDisplaysState(bool val)  { showDisplays  = val; }
    void SetMessagesState(bool warnSt, bool mileSt, bool landSt, bool verbSt, bool dispSt) {
	showWarnings  = warnSt,
	    showMilestones = mileSt,
	    showLandmarks = landSt,
	    showVerboses  = verbSt,
	    showDisplays  = dispSt;
    }

    // methods for querying about the state enabled/disabled of messages
    bool GetShowWarningsState()   { return showWarnings; }
    bool GetShowMilestonesState() { return  showMilestones; }
    bool GetShowLandmarksState()  { return showLandmarks; }
    bool GetShowVerbosesState()   { return showVerboses; }
    bool GetShowDisplaysState()   { return showDisplays; }
    
protected:

    // boolean members for messages by type enabled/disabled state.
    bool showWarnings;
    bool showMilestones;
    bool showLandmarks;
    bool showVerboses;
    bool showDisplays;
};

#endif //ifdef wip_WipReporter_h
