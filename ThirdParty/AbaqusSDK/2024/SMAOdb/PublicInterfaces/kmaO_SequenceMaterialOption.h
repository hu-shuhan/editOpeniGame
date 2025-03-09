//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_SequenceMaterialOption_h
#define kmaO_SequenceMaterialOption_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <kmaO_MaterialOption.h>
#include <kmaC_MatOptionListShortcut.h>
#include <cow_List.h>

// Forward declarations

// Low level list implementation

COW_LIST_DECL(kmaO_MaterialOptionCOW, kmaO_SequenceMaterialOptionImpl);

class kmaO_SequenceMaterialOption  : public mem_AllocationOperators
{
  public:
    
    kmaO_SequenceMaterialOption();
   ~kmaO_SequenceMaterialOption();
    
    kmaO_MaterialOption& get(int index);
    const kmaO_MaterialOption& constGet(int index) const;
    const kmaO_MaterialOption& operator[](int index) const;
    void append(const kmaO_MaterialOption& option);
    void append(const kmaO_SequenceMaterialOption& optionSequence);
    int size() const;
    void clearAll();

    // Undocumented and unsupported
    kmaO_SequenceMaterialOption(
	const kmaC_MaterialOptionListShortcut& sc);
    kmaO_SequenceMaterialOption& operator=(
	const kmaO_SequenceMaterialOption& rhs);
    kmaO_SequenceMaterialOption(
	const kmaC_MaterialOptionList& l);
    kmaC_MaterialOptionListShortcut Shortcut() const { 
	return m_shortcut; 
    }
    const kmaO_MaterialOption& GetByReference(int index) const;
    
  private:

    bool m_standalone;
    mutable kmaO_SequenceMaterialOptionImpl m_array;
    kmaC_MaterialOptionListShortcut m_shortcut;
};

#endif
