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
// File Name: odb_Text.h

#ifndef odb_Text_h
#define odb_Text_h

//
// SECTION: System includes
//

//
// Begin local includes
//
#include <odb_Enum.h>
#include <odb_String.h>
#include <odb_Types.h>
#include <odb_Annotation.h>
#include <cow_COW.h>

// Forward declarations

class ddb_Text;

//  
// Class: odb_Text
// 
class odb_Text
    : public odb_Annotation
{
public:   
    explicit odb_Text( const odb_Annotation &annotation );

    // Access methods
    odb_String text() const;
    double offsetX() const;
    double offsetY() const;
    double rotationAngle() const;
    odb_String font() const;
    odb_String color() const; 
    odb_Enum::odb_BackgroundStyleEnm backgroundStyle() const; 
    odb_String backgroundColor() const;
    bool box() const;
    odb_Enum::odb_LineJustificationEnm justification() const;

    // Set methods
    void setText(const odb_String& text);
    void setOffsetX(double offsetX);
    void setOffsetY(double offsetY); 
    void setRotationAngle(double rotationAngle);
    void setFont(const odb_String& font);
    void setColor(const odb_String& color);
    void setBackgroundStyle(odb_Enum::odb_BackgroundStyleEnm backgroundStyle);
    void setBackgroundColor(const odb_String& backgroundColor);
    void setBox(bool box);
    void setLineJustification(odb_Enum::odb_LineJustificationEnm lineJustification); 

public:  // Undocumented and unsupported
    odb_Text( const ddb_Text &text);
    static unsigned int typeIdentifier();

private:
    const ddb_Text& constTextAnnotation() const;
    ddb_Text& textAnnotation();
};

#endif
