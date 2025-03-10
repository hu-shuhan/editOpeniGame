//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_Arrow_h
#define odb_Arrow_h
//
// Begin local includes
//
#include <odb_Enum.h>
#include <odb_String.h>
#include <odb_Types.h>
#include <odb_Annotation.h>
#include <cow_COW.h>

// Forward declarations

class ddb_Arrow;

//  
// Class: odb_Arrow
// 
class odb_Arrow
    : public odb_Annotation
{
public:   
    explicit odb_Arrow( const odb_Annotation &annotation );

    // Access methods
    double startPointX() const;
    double startPointY() const; 
    double endPointX() const;
    double endPointY() const;
    odb_Enum::odb_ArrowHeadStyleEnm startHeadStyle() const; 
    odb_Enum::odb_ArrowHeadStyleEnm endHeadStyle() const;
    double startGap() const;  
    double endGap() const;
    odb_String color();
    odb_Enum::odb_LineStyleEnm lineStyle() const;
    odb_Enum::odb_LineThicknessEnm lineThickness() const;

    // Set methods
    void setStartPointX(double startPointX);  
    void setStartPointY(double startPointY); 
    void setEndPointX(double endPointX);    
    void setEndPointY(double endPointY);
    void setStartHeadStyle(odb_Enum::odb_ArrowHeadStyleEnm startHeadStyle);
    void setEndHeadStyle(odb_Enum::odb_ArrowHeadStyleEnm endHeadStyle);
    void setStartGap(double startGap); 
    void setEndGap(double endGap);
    void setColor(const odb_String& color);
    void setLineStyle(odb_Enum::odb_LineStyleEnm lineStyle);
    void setLineThickness(odb_Enum::odb_LineThicknessEnm lineThickness);

    // Methods
    void translateStartPoint(double x, double y) ;
    void translateEndPoint(double x, double y);

public:  // Undocumented and unsupported
    odb_Arrow( const ddb_Arrow &arrow );
    static unsigned int typeIdentifier();

private:
    const ddb_Arrow& constArrowAnnotation() const;
    ddb_Arrow& arrowAnnotation();
};

#endif
