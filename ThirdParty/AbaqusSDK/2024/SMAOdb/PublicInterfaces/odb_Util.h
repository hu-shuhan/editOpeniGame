//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_Util_h
#define odb_Util_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <cow_2DArrayDouble.h>
#include <odb_Types.h>
#include <odb_Enum.h>
// Forward declarations
class ddb_Assembly;
class odb_Set;
class odb_Instance;
class odb_Mesh;
class ddb_Set;
class rgnC_Region;
struct odiK_SubsetStruct;
template <class A> class cow_List;
typedef cow_List<odiK_SubsetStruct> odiK_SubsetStructList;
typedef odiK_SubsetStructList ddb_SubsetStructList;
class odb_DatumCsys;
class dtmC_DatumRef;

class odb_Util  : public mem_AllocationOperators
{
public:

    static void EnforceComponentNames( const odb_String& name,
                                       odb_SequenceString& componentLabels,
                                       const odb_SequenceString& validLabels);

    static void CheckTransform(
                      const odb_SequenceSequenceSequenceFloat& coordSystem,
                      int numDat );

    // model util
    static bool AssemblyHasInstanceName( const ddb_Assembly& assy,
                                         const cow_String& instanceName );

    static odb_String InstanceNameFromMeshIndex( const ddb_Assembly assy,
                                                 int meshIndex );

    static odb_Instance InstanceFromMeshIndex( const ddb_Assembly assy,
                                               int meshIndex );
    
    static odb_String InitialStepName();

    static void SplitSurfaceByType( const odb_SequenceInt& labels,
                                    const odb_SequenceElementFace& faces,
                                    const odb_Mesh& mesh,
                                    ddb_SubsetStructList& subsets );
    
    // General utilities
    static cow_2DArrayDouble Seq2Cow(const odb_SequenceSequenceFloat&);
    static cow_2DArrayDouble Seq2Cow(const odb_SequenceSequenceDouble&);
    static cow_2DArrayDouble Seq2CowIrregular( const odb_SequenceSequenceFloat&);
    static cow_2DArrayDouble Seq2CowIrregular( const odb_SequenceSequenceDouble&);
    static odb_SequenceSequenceFloat Cow2Seq(const cow_2DArrayDouble&);
    static odb_SequenceSequenceDouble Cow2SeqDouble(const cow_2DArrayDouble&);

    static odb_Set RegionToOdbSet( const rgnC_Region& dset );
    static odb_DatumCsys DatumToOdbDatumCsys( const dtmC_DatumRef& dref, const cow_String& shortcutPath );

    static odb_SequenceInt SortUndupl(const odb_SequenceInt&); // sort and remove duplicates

};

#endif // odb_Util_h
