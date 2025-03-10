//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_SequenceAnalyticSurfaceSegment_h
#define odb_SequenceAnalyticSurfaceSegment_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odiK_AnalSurfSegListShortcut.h>
#include <cow_List.h>
#include <odb_AnalyticSurfaceSegment.h>
// Forward declarations

COW_LIST_DECL(odb_AnalyticSurfaceSegmentCOW, odb_AnalyticSurfaceSegmentListImpl);

class odb_SequenceAnalyticSurfaceSegment : public mem_AllocationOperators
{
public:
    odb_SequenceAnalyticSurfaceSegment();
    virtual ~odb_SequenceAnalyticSurfaceSegment();
    
    odb_AnalyticSurfaceSegment& get( int index );
    const odb_AnalyticSurfaceSegment& constGet( int index ) const;
    const odb_AnalyticSurfaceSegment& operator[]( int index ) const;
    void Start( const odb_SequenceFloat& origin );
    void Line( const odb_SequenceFloat& origin);
    void Circle(const odb_SequenceFloat& center, const odb_SequenceFloat& endPoint);
    void Parabola(const odb_SequenceFloat& middlePoint, const odb_SequenceFloat& endPoint);
    void append( const odb_AnalyticSurfaceSegment& ass );
    void append( const odb_SequenceAnalyticSurfaceSegment& seq );
    int size() const;
    void clearAll();

    // Undocumented and unsupported
    const odb_AnalyticSurfaceSegment& GetByReference( int i ) const;
    odb_SequenceAnalyticSurfaceSegment( const shp_AnalyticSurfaceSegmentListShortcut& cps );
    odb_SequenceAnalyticSurfaceSegment& operator=( const odb_SequenceAnalyticSurfaceSegment& rhs );
    odb_SequenceAnalyticSurfaceSegment( const shp_AnalyticSurfaceSegmentList& l );

private:
    mutable odb_AnalyticSurfaceSegmentListImpl m_array;    
    shp_AnalyticSurfaceSegmentListShortcut m_shortcut;
    bool m_standalone;
};

#endif // odb_SequenceAnalyticSurfaceSegment_h
