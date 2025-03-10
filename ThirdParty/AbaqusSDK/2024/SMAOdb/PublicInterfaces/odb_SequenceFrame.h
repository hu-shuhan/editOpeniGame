//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_SequenceFrame_h
#define odb_SequenceFrame_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_Frame.h>
#include <cow_List.h>
// End local includes

// Forward Declarations
class ddb_Step;
COW_COW_DECL(ddb_Step);
class ddb_Assembly;
COW_ARRAYCOW_DECL(ddb_Assembly);

class odb_SequenceFrame : public mem_AllocationOperators
{
public:
  int size() const;
  odb_Frame& get(int frameNo) const;
  const odb_Frame& constGet(int frameNo) const;
  odb_Frame& operator[](int frameNo) const;
  void release(int frameNo) const;
  void release() const;

public: // Undocumented and Unsupported
  odb_SequenceFrame(const ddb_Step& step, const ddb_Assembly& rootAssy, odb_Enum::odb_DomainEnum domain, const odb_Step& parent);
  odb_SequenceFrame();
  odb_SequenceFrame(const odb_SequenceFrame& seqFrame);
  ~odb_SequenceFrame();
  void append(const odb_Frame& frame);
  bool remove(int frameNo);

private:
  ddb_StepCOW                 m_step;
  ddb_AssemblyCOW             m_rootAssy;
  odb_Enum::odb_DomainEnum m_domain;
  odb_StepCOW m_parent;
  cow_MapInt2FrameCOW m_frameMap;

};

#endif // #ifndef odb_SequenceFrame_h
