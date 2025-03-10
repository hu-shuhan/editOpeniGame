//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
///////////////////////////////////////////////////////////////////////////////
//
// File Name: ddb_Text.h

#ifndef ddb_Text_h
#define ddb_Text_h

//
// SECTION: System includes
//

//
// Begin local includes
//
#include <BasicConstants.h>
#include <udd_AnnotationShortcut.h>
#include <annC_Annotation.h>
#include <annC_Anchor.h>
#include <annC_Text.h>
#include <annC_Enums.h>
#include <cow_ListListFloat.h>
#include <cow_COW.h>
#include <ddb_Annotation.h>
#include <cow_String.h>
//  
// Class: ddb_Text
// 

class ddb_Text: public ddb_Annotation
{
public:
    ddb_Text(const udd_AnnotationOdbShortcut&, unsigned id);
   ~ddb_Text();

    virtual ddb_Annotation *Copy() const { return new ddb_Text(*this); }

    //set methods
    void setText(const cow_String&);
    void setOffsetX(double);
    void setOffsetY(double);
    void setAnchor(const annC_AnchorCOW& );
    void setReferencePoint(const annC_AnchorCOW& );
    void setRotationAngle(double);
    void setFont(const cow_String&);
    void setColor(const cow_String&);
    void setBackgroundStyle(annC_BackgroundStyleEnm);
    void setBackgroundColor(const cow_String&);
    void setBox(bool);
    void setLineJustification(annC_JustificationEnm);

	//get methods
	const cow_String& text() const;
    double offsetX() const;
    double offsetY() const;
    const annC_AnchorCOW& anchor() const;
    const annC_AnchorCOW&  referencePoint() const;
    double rotationAngle() const;
    const cow_String& font() const;
    const cow_String& color() const;
    annC_BackgroundStyleEnm backgroundStyle() const ;
    const cow_String& backgroundColor() const;
    bool box() const;
    annC_JustificationEnm lineJustification() const;

    // Helper function
    void  setTextHelper( const annC_Text& annotation );

private:
    bool operator== ( ddb_Text &rhs );
    bool operator!= ( ddb_Text &rhs );
};

#endif
