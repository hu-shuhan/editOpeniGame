//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
// -*- Mode: C++ -*-
#ifndef annC_Arrow_h
#define annC_Arrow_h

// Begin local includes
#include <annC_Annotation.h>
#include <annC_Anchor.h>
#include <annC_Enums.h>
#include <cow_String.h>

class annC_Arrow
    : public annC_Annotation
{
public:

    // Constructor.
    annC_Arrow();
  
    // Establishes a new identity in DDB.
    virtual void NewUid();
    
    // Virtual copy.
    virtual annC_Annotation *Copy() const;

    // Services
    virtual void Translate( double x, double y );

    // Members

    double startPointX;
    double startPointY;
    double endPointX;
    double endPointY;
    annC_AnchorCOW startAnchor;
    annC_AnchorCOW endAnchor;
    annC_ArrowHeadStyleEnm startHeadStyle;
    annC_ArrowHeadStyleEnm endHeadStyle;
    double startGap;
    double endGap;
    cow_String color;
    annC_LineStyleEnm lineStyle;
    annC_LineThicknessEnm lineThickness;

    // Static members

    static omn_FixedString constructorCmd();
    static omn_FixedString startPointCmd();
    static omn_FixedString endPointCmd();
    static omn_FixedString startAnchorCmd();
    static omn_FixedString endAnchorCmd();
    static omn_FixedString startHeadStyleCmd();
    static omn_FixedString endHeadStyleCmd();
    static omn_FixedString startGapCmd();
    static omn_FixedString endGapCmd();
    static omn_FixedString colorCmd();
    static omn_FixedString lineStyleCmd();
    static omn_FixedString lineThicknessCmd();

    static omn_FixedString translateStartPointCmd();
    static omn_FixedString translateEndPointCmd();

public: // Messaging/database interface

    virtual typ_typeTag     DynTypeId() const;
    static typ_typeTag      TypeId();

    // DDB persistence
    annC_Arrow( const cls_ReadVisitor& rv );
    virtual void DBWrite( const cls_WriteVisitor& wv ) const;
    static void InitializeMetadata( cls_ClassRegistrar& reg );
    
private:
    cls_Uid m_ClsUid;
};


#endif

