//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
// omu_VarrayDef.h
// Variable-length arrays of T.
// To be included by each template instantiation .h file.

#if !defined(omu_VarrayDef_h)
#define omu_VarrayDef_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <omi_types.h>

template<class T>
class omu_Varray  : public mem_AllocationOperators
{

public:
    omu_Varray();
    omu_Varray(int initial_size, double grow_ratio = 2.0);
    ~omu_Varray();

    T& operator[](int i);    // Add if necessary
    T operator[](int i) const; // Return voidvalue if invalid
    T Pop(void);         // Remove(Length-1); return voidvalue if empty

    bool Get(int i, T *vp) const;
    bool Set(int i, T v);
    bool Add(T v);    // Add one past end
    bool Insert(int i, T v);   // Subsequent elements shifted up
    bool Remove(int i, T *vp = 0);
    const int& Length(void) const    // 0 when empty
    { return length; }

    void ClearAll();                // Length becomes 0
    int FindFirst(T const& v) const;  // Return Length() on failure.
    int FindLast(T const& v) const;   // Return Length() on failure.

    T* GetAll(void) const;    // Caller must delete array afterwards

    // DO NOT call this method unless for omu_VarrayInt
    const T* ConstGetAll() const;

    void SetGrowthRatio(double gr) {growth_ratio=gr;} // Set growth ratio
    bool SetSize(int size_in);  // Grow the Varray to this size
    int InitialSize() const { return initial_size; }
    int Size() const { return size; }
    double GrowthRatio() const { return growth_ratio; }
  
private:

    bool Grow(void);

    // Copy constructor and assignment operator declared private.
    omu_Varray(const omu_Varray<T>&);
    omu_Varray<T>& operator=(const omu_Varray<T>&);

    static T voidvalue;       // Add this value by default

    int initial_size;   // Initial size to be allocated, dafault 16
    double growth_ratio;   // Growth_Ratio, default 2.0

    int length;
    int size;
    T *data;
};

#endif /* omu_VarrayDef_h */
