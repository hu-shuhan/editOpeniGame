//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
// -*- Mode: C++ -*-
#ifndef annC_Text_h
#define annC_Text_h

// Begin local includes
#include <annC_Annotation.h>
#include <annC_Anchor.h>
#include <annC_Enums.h>
#include <cow_String.h>

class annC_Text
    : public annC_Annotation
{
public:

    // Constructor.
    annC_Text();
   
    // Establishes a new identity in DDB.
    virtual void NewUid();
    
    // Virtual copy.
    virtual annC_Annotation *Copy() const;

    // Services
    virtual void Translate( double x, double y );

    // Members

    cow_String text;
    double offsetX;
    double offsetY;
    annC_AnchorCOW anchor;
    annC_AnchorCOW referencePoint;
    double rotationAngle;
    cow_String font;
    cow_String color;
    annC_BackgroundStyleEnm backgroundStyle;
    cow_String backgroundColor;
    bool box;
    annC_JustificationEnm justification;

    // Static members

    static omn_FixedString constructorCmd();
    static omn_FixedString textCmd();
    static omn_FixedString offsetCmd();
    static omn_FixedString anchorCmd();
    static omn_FixedString referencePointCmd();
    static omn_FixedString rotationAngleCmd();
    static omn_FixedString fontCmd();
    static omn_FixedString colorCmd();
    static omn_FixedString backgroundStyleCmd();
    static omn_FixedString backgroundColorCmd();
    static omn_FixedString boxCmd();
    static omn_FixedString justificationCmd();

    static omn_FixedString widthCmd();
    static omn_FixedString heightCmd();

public: // Messaging/database interface

    virtual typ_typeTag     DynTypeId() const;
    static typ_typeTag      TypeId();

    // DDB persistence
    annC_Text( const cls_ReadVisitor& rv );
    virtual void DBWrite( const cls_WriteVisitor& wv ) const;
    static void InitializeMetadata( cls_ClassRegistrar& reg );
    
private:
    cls_Uid m_ClsUid;
};



#endif

