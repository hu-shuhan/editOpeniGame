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
// File Name: ddb_Arrow.h

#ifndef ddb_Arrow_h
#define ddb_Arrow_h

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
#include <annC_Arrow.h>
#include <annC_Enums.h>
#include <cow_ListListFloat.h>
#include <cow_COW.h>
#include <ddb_Annotation.h>
#include <cow_String.h>
//  
// Class: ddb_Arrow
// 

class ddb_Arrow: public ddb_Annotation
{
public:
    ddb_Arrow( const udd_AnnotationOdbShortcut&, unsigned id );
   ~ddb_Arrow();

    virtual ddb_Annotation *Copy() const { return new ddb_Arrow(*this); }

    //set methods
    void setStartPointX(double);
    void setStartPointY(double);
    void setEndPointX(double);
    void setEndPointY(double);
    void setStartAnchor(const annC_AnchorCOW& );
    void setEndAnchor(const annC_AnchorCOW&);
    void setStartHeadStyle(annC_ArrowHeadStyleEnm);
    void setEndHeadStyle(annC_ArrowHeadStyleEnm);
    void setStartGap(double);
    void setEndGap(double);
    void setColor(const cow_String&);
    void setLineStyle(annC_LineStyleEnm);
    void setLineThickness(annC_LineThicknessEnm);

	//Access methods
	double startPointX() const;
    double startPointY() const; 
    double endPointX() const;
    double endPointY() const;
	const annC_AnchorCOW& startAnchor() const;
    const annC_AnchorCOW& endAnchor() const;
    annC_ArrowHeadStyleEnm startHeadStyle() const; 
    annC_ArrowHeadStyleEnm endHeadStyle() const;
    double startGap() const;  
    double endGap() const;
    const cow_String& color() const;
    annC_LineStyleEnm lineStyle() const;
    annC_LineThicknessEnm lineThickness() const;
    // Helper function
    void  setArrow( const annC_Arrow& annotation );

private:
    bool operator== ( ddb_Arrow &rhs );
    bool operator!= ( ddb_Arrow &rhs );
};

#endif
