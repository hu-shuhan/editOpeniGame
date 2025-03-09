//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_UserData_h
#define odb_UserData_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_UserXYData.h>
#include <odb_String.h>
#include <odb_Enum.h>
#include <odb_Arrow.h>
#include <odb_Text.h>
#include <odb_UserXYDataRepository.h>
#include <odb_Annotation.h>
#include <odb_AnnotationRepository.h>
#include <ddb_UserData.h>
#include <annC_Arrow.h>
// End local includes

// Forward declarations
class annC_Annotation;

class odb_UserData : public mem_AllocationOperators
{
public:
  
    // UserXYData constructor
    odb_UserXYData& XYData(const odb_String& name,
                           const odb_Enum::odb_XYSourceTypeEnm& sourceType,
                           const odb_String& sourceFileName,
                           const odb_String& sourceDescription,
                           const odb_String& contentDescription,
                           const odb_String& positionDescription,
                           const odb_String& xAxisLabel,
                           const odb_String& yAxisLabel,
                           const odb_String& legendLabelx );

    // Annotation constructor
    odb_Annotation CopyAnnotation(const odb_String& name, const annC_Annotation& annotation);
    // Arrow constructor
    odb_Arrow Arrow(const odb_String& name, double startPointX=0.0, double startPointY=0.0,
		     double endPointX=0.0, double endPointY=0.0,
		     odb_Enum::odb_RelativePositionEnm startAnchor=odb_Enum::BOTTOM_LEFT, 
                     odb_Enum::odb_RelativePositionEnm endAnchor=odb_Enum::BOTTOM_LEFT, 
                     odb_Enum::odb_ArrowHeadStyleEnm startHeadStyle=odb_Enum::NO_HEAD, 
		     odb_Enum::odb_ArrowHeadStyleEnm endHeadStyle=odb_Enum::FILLED_ARROW,
		     double startGap=0.0, double endGap=0.0,const odb_String& color=odb_String("White"),
                     odb_Enum::odb_LineStyleEnm lineStyle=odb_Enum::SOLID,
                     odb_Enum::odb_LineThicknessEnm lineThickness=odb_Enum::VERY_THIN);
    // Text constructor
    odb_Text Text(const odb_String& name,  
		     const odb_String& text=odb_String(""), double offsetX=0.0, double offsetY=0.0,
                     odb_Enum::odb_RelativePositionEnm anchor=odb_Enum::BOTTOM_LEFT, 
                     odb_Enum::odb_RelativePositionEnm referencePoint=odb_Enum::BOTTOM_LEFT,
                     double rotationAngle=0.0,const odb_String& font=odb_String("-*-helvetica-bold-r-normal--12-*"),const odb_String& color=odb_String("White"),
		     odb_Enum::odb_BackgroundStyleEnm backgroundStyle=odb_Enum::OTHER,
                     const odb_String& backgroundColor=odb_String("Black"),
                     bool box=odb_Enum::OFF,odb_Enum::odb_LineJustificationEnm justification=odb_Enum::JUSTIFY_LEFT);
    bool update();

    // member access
    odb_UserXYData& xyDataObjects(const odb_String& xyName);
    odb_UserXYDataRepository& xyDataObjects();
    odb_Annotation& annotationObjects(const odb_String& xyName);
    odb_AnnotationRepository& annotationObjects();


private:
    ddb_UserDataCOW m_userData;
    odb_UserXYDataRepository m_userXYDataContainer;
    odb_AnnotationRepository m_annotationContainer;
    bool isRead;

public:   // undocumented and unsupported
    odb_UserData();
    odb_UserData( const ddb_UserData& userData );

    void Read();
    void ReadUserXYDataCon();
    void ReadUserXYData( const odb_String& xyName );

    ddb_UserData& userData() { return m_userData.Get(); }
};

#endif // odb_UserData_h
