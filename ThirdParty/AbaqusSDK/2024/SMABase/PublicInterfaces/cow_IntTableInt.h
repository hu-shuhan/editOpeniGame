//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
// -*- mode: c++ -*- //
#ifndef cow_IntTableInt_h
#define cow_IntTableInt_h

// Begin local includes
#include <mem_AllocationOperators.h>

#include <cow_COW.h>
#include <omi_IntTableInt.h>

// Forward declaration

class cow_IntTableIntIT;

// Class definition

COW_COW_DECL(omi_IntTableInt);

class cow_IntTableInt  : public mem_AllocationOperators
{
  friend class cow_IntTableIntIT;

  public:

  cow_IntTableInt();

  bool       Remove(const int& key); 
  bool       Insert(const int& key, const int& value);

  const int& ConstGet(const int& key) const;
  bool       ConstGetValue(const int& key, int& val) const;
  int&       Get(const int& key);

  bool       IsMember(const int& key) const;
  int        Size() const;
  int        SizeOfNegatives() const;
  bool       IsEmpty() const;
  void       Clear();

private:

    omi_IntTableIntCOW tbl;
};

// Const Iterator over Map

class cow_IntTableIntIT  : public mem_AllocationOperators
{
  public:

    cow_IntTableIntIT(const cow_IntTableInt& t): 
        iter(t.tbl.ConstGet()) {}

    void First() { iter.First(); }
    void Last()  { iter.Last(); }

    void Next()  { iter.Next(); }

    bool IsDone() const { return iter.IsDone(); }

    const int& CurrentValue() const {
        return iter.CurrentValue();
    }

    int CurrentKey() const { return iter.CurrentKey(); }

  private:

    omi_IntTableIntIT iter;
};

typedef cow_IntTableIntIT cow_IntTableIntSIT;

#endif // cow_IntTableInt_h
