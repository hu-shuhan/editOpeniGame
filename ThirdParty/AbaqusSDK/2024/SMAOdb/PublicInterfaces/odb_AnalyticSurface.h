//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_AnalyticSurface_h
#define odb_AnalyticSurface_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odiK_AnalyticSurfaceShortcut.h>
#include <shp_AnalyticSurface.h>
#include <odb_SeqAnalSurfSegment.h>
#include <odb_String.h>
#include <odb_Types.h>
// Forward declarations

class odb_AnalyticSurface  : public mem_AllocationOperators
{
    
public:
    odb_AnalyticSurface( const shp_AnalyticSurfaceShortcut& shortcut );
    ~odb_AnalyticSurface();
    odb_String name() const;
    odb_String type() const;
    double filletRadius() const;
    odb_SequenceSequenceFloat localCoordData() const;
    odb_SequenceAnalyticSurfaceSegment segments() const;
    // undocumented and unsupported
    static shp_AnalyticSurface* ConstructObject( odb_String name, 
                                                 odb_String type,
                                                 double filletRadius,
                                                 const odb_SequenceAnalyticSurfaceSegment& segments = odb_SequenceAnalyticSurfaceSegment(),
                                                 const odb_SequenceSequenceFloat& localCoordData = odb_SequenceSequenceFloat() );
    shp_AnalyticSurfaceShortcut Shortcut() const { return m_shortcut; };
protected:
    shp_AnalyticSurfaceShortcut m_shortcut;
private:

};

#endif // odb_AnalyticSurface_h
