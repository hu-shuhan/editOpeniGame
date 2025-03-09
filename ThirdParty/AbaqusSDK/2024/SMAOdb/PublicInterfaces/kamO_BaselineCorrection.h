//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kamO_BaselineCorrection_h
#define kamO_BaselineCorrection_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <kamC_BaselineCorrectionShortcut.h>
#include <odb_Types.h>
// Forward declarations
class kamC_BaselineCorrection;

typedef cow_CountedHld<kamC_BaselineCorrection> kamC_BaselineCorrectionHld;
class kamO_BaselineCorrection : public mem_AllocationOperators
{
public:
    kamO_BaselineCorrection( const kamC_BaselineCorrectionShortcut& shortcut );
    kamO_BaselineCorrection(odb_SequenceDouble intervals=odb_SequenceDouble());

    kamO_BaselineCorrection( const kamO_BaselineCorrection& copy );

    virtual ~kamO_BaselineCorrection();
    odb_SequenceDouble intervals() const;
    kamO_BaselineCorrection& operator=( const kamO_BaselineCorrection& rhs );

    kamC_BaselineCorrectionShortcut Shortcut() const { return shortcut; }

    bool hasValue() const { return shortcut.HasValue(); }

    const kamC_BaselineCorrection* GetPointer() const { return pointer.GetPtr();} 

    virtual kamO_BaselineCorrection* Copy() const;

protected:
    kamC_BaselineCorrectionShortcut shortcut;
private:
    kamC_BaselineCorrectionHld pointer;
};

#endif
