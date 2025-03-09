//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2015
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbeO_CDCTerm_h
#define kbeO_CDCTerm_h
///////////////////////////////////////////////////////////////////////////////
// kbeO_CDCTerm
//

// Begin local includes
#include <cow_ArrayCOW.h>
#include <odb_Types.h>
#include <mem_AllocationOperators.h>
#include <kbeC_CDCTermShortcut.h>
#include <odb_String.h>
// Forward declarations
class kbeO_ConnectorOptions;
class kbeC_CDCTerm;
typedef cow_CountedHld<kbeC_CDCTerm> kbeC_CDCTermHld;

class kbeO_CDCTerm: public mem_AllocationOperators
{
  public:
    kbeO_CDCTerm( const kbeC_CDCTermShortcut& shortcut );
    kbeO_CDCTerm( odb_String termOperator = "RSS",
                  odb_String termSign = "POSITIVE",
                  bool localDependency = false,
                  odb_String indepCompType = "POSITION",
                  bool tempDependency = false,
                  int fieldDependencies = 0,
                  const odb_SequenceInt& intrinsicComponents = odb_SequenceInt(),
                  const odb_SequenceInt& indepComponents = odb_SequenceInt(),
                  const odb_SequenceSequenceDouble& table = odb_SequenceSequenceDouble() );
    kbeO_CDCTerm( const kbeO_CDCTerm& copy );
    virtual kbeO_CDCTerm* Copy() const;
    kbeO_CDCTerm& operator=( const kbeO_CDCTerm& rhs );

    virtual ~kbeO_CDCTerm();
    odb_String termOperator() const;
    odb_String termSign() const;
    bool localDependency() const;
    odb_String indepCompType() const;
    bool tempDependency() const;
    int fieldDependencies() const;
    odb_SequenceInt intrinsicComponents() const;
    odb_SequenceInt indepComponents() const;
    odb_SequenceSequenceDouble table() const;
    kbeO_ConnectorOptions options() const;
    kbeO_ConnectorOptions Options( bool useBehRegSettings = true,
                                   bool regularize = true,
                                   bool defaultTolerance = true,
                                   double regularization = 0.03,
                                   bool defaultRateFactor = true,
                                   double rateFactor = 0.9,
                                   odb_String interpolation = "LINEAR",
                                   bool useBehExtSettings = true,
                                   odb_String extrapolation = "CONSTANT" );
    void SetPointer( kbeC_CDCTerm* ptr) { pointer = ptr; }
    const kbeC_CDCTerm* GetPointer() const;
    kbeC_CDCTermShortcut Shortcut() const { return shortcut; } 

    bool hasValue() const { return shortcut.HasValue(); }

    static unsigned int typeIdentifier();
    unsigned int subTypeIdentifier() const;

protected:
    kbeC_CDCTermShortcut shortcut;
    kbeC_CDCTermHld pointer;
};

COW_ARRAYCOW2_DECL(kbeO_CDCTerm, cow_Virtual);

#endif // kbeO_CDCTerm_h
