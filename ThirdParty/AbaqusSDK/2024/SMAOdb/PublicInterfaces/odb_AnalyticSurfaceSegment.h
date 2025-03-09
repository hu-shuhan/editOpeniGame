//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_AnalyticSurfaceSegment_h
#define odb_AnalyticSurfaceSegment_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <shp_AnalyticSurfaceSegment.h>
#include <odiK_AnalSurfSegShortcut.h>
#include <odb_Types.h>
#include <cow_ArrayCOW.h>

// Forward declarations
typedef cow_CountedHld<shp_AnalyticSurfaceSegment> shp_AnalyticSurfaceSegmentHld;

class odb_AnalyticSurfaceSegment  : public mem_AllocationOperators
{
    
public:
    odb_AnalyticSurfaceSegment( const odb_String& type,
                                const odb_SequenceSequenceFloat& data );
    odb_AnalyticSurfaceSegment( const odb_AnalyticSurfaceSegment& copy );
    odb_AnalyticSurfaceSegment& operator=( const odb_AnalyticSurfaceSegment& rhs );
    virtual ~odb_AnalyticSurfaceSegment();
    odb_String type() const;
    odb_SequenceSequenceFloat data() const;
    // undocumented and unsupported
    odb_AnalyticSurfaceSegment();
    odb_AnalyticSurfaceSegment( const shp_AnalyticSurfaceSegmentShortcut& shortcut );
    shp_AnalyticSurfaceSegmentShortcut Shortcut () const { return shortcut; }
    bool hasValue() const { return shortcut.HasValue(); }
    const shp_AnalyticSurfaceSegment* GetPointer() const; 
    void SetPointer( shp_AnalyticSurfaceSegment* ptr) { pointer = ptr; }
protected:
    shp_AnalyticSurfaceSegmentShortcut shortcut;
    shp_AnalyticSurfaceSegmentHld pointer;

};

COW_ARRAYCOW_DECL(odb_AnalyticSurfaceSegment);

#endif // odb_AnalyticSurfaceSegment_h
