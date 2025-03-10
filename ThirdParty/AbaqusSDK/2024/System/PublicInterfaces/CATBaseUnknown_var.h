#include "CATBaseUnknown_var_required.h"
#ifndef __CATBaseUnknown_var
#define __CATBaseUnknown_var    42601

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#include <stdio.h>
#include "CATBaseUnknown.h"
#include "CATMacForCodeActivation.h"  // CATIAV5Level

/**
 * @nodoc
 */
typedef IUnknown *  IUnknown_ptr;

/**
 * @nodoc
 */
#define NULL_var (static_cast<CATBaseUnknown*>(nullptr))

/**
 * Base class for handler.
 * <b>Role</b>: Base class for the CORBA's object reference variable type for an
 * interface, also known as handler, and smart interface pointer with COM.
 */
class CATBaseUnknown_var
{
   public:
/**
 * Constructs an empty handler. 
 */
      CATBaseUnknown_var();

      ~CATBaseUnknown_var();

/**
 * Constructs a handler from a pointer.
 * @param iPtr The pointer to the @href CATBaseUnknown instance to use. 
 */
      CATBaseUnknown_var(CATBaseUnknown * iPtr);
      
/**
 * Move constructor.
 */
      CATBaseUnknown_var(CATBaseUnknown_var&& other) noexcept;

/**
 * Copy constructor.
 * @param iRef The reference of the CATBaseUnknown_var instance to copy.
 */
      CATBaseUnknown_var(const CATBaseUnknown_var & iRef);

/**
 * Assignment operator from CATBaseUnknown_var reference.
 * @param iRef The reference of the CATBaseUnknown_var instance to copy.
 * @return The reference of the CATBaseUnknown_var instance valued.
 */
      ExportedByJS0CORBA CATBaseUnknown_var & __stdcall operator=(const CATBaseUnknown_var & iRef);
      
/**
 * Move assignment operator.
 */
      ExportedByJS0CORBA CATBaseUnknown_var& operator=(CATBaseUnknown_var&& other);

/** @nodoc Returns the stored pointer. */
      CATBaseUnknown* get() const { return static_cast<CATBaseUnknown*>(pointer); }

      // Comparison operators
/**
 * Equality operator.
 * Starting from C++20, also used for inequality comparisons.
 * @param iPt A pointer to a @href CATBaseUnknown to compare.
 * @return
 * <b>Legal values</b>:
 * <dl>
 * <dt>0</dt><dd> The handler and iPt are pointing the same object</dd>
 * <dt>1</dt><dd> The handler and iPt are not pointing the same object</dd>
 * </dl>
 */
      bool operator==(CATBaseUnknown *iPt) const { return equal_to(pointer, iPt); }
      
      
#if (__cplusplus < 202002L)
      /** @nodoc Starting from C++20, not required anymore */
      bool operator!=(CATBaseUnknown *iPt) const { return !(*this == iPt); }
#else  // C++20
      /** @nodoc operator required to disambiguate conversions between two "_var" types */
      bool operator==(const CATBaseUnknown_var & rhs) const {
        return equal_to(pointer, rhs.pointer);
      }
#endif

/**
 * Logical not operator.
 * @return
 * <b>Legal values</b>:
 * <dl>
 * <dt>0</dt><dd> The handler points on a valid object</dd>
 * <dt>1</dt><dd> The handler does not point on a valid object</dd>
 * </dl>
 */
      ExportedByJS0CORBA int __stdcall operator!() const;

/**
 * Type cast operator to a CATBaseUnknown instance pointer.
 */
      operator CATBaseUnknown*() const;

/**
 * Pointer to member operator.
 */
      CATBaseUnknown * operator->() const;
      

/** @nodoc @see CATBaseUnknown::IsEqual */
      bool IsEqual(IUnknown *rhs) const {
        return equal_to(pointer, rhs);
      }

/**
 * For illegal operation.
 */
#if defined(CATIAR427)
    protected:
      [[noreturn]]
#endif
      ExportedByJS0CORBA void ThrowErrorNullVar() const;
      
   protected:
/** @nodoc */
      ExportedByJS0CORBA void __stdcall CastTo(IUnknown *ipt, const GUID &guid);
      
/** @nodoc */
      IUnknown * GetPointer() const;

   private:
/** @nodoc */
      IUnknown *pointer;
   
/** @nodoc */
    static ExportedByJS0CORBA bool equal_to(IUnknown*, IUnknown*);
    
#if (__cplusplus >= 202002L)

#if !defined(CATIAR427)
/** @nodoc */
    static ExportedByJS0CORBA bool equal_to_symmetric(IUnknown*, IUnknown*);

/** @nodoc */
    friend bool operator==(CATBaseUnknown *lhs, const CATBaseUnknown_var & rhs) {
      return equal_to_symmetric(lhs, rhs.pointer);
    }
#endif  // Pre-R426

/** @nodoc */
    template <typename _Ty, typename _Tz>
    requires(std::is_base_of_v<CATBaseUnknown_var, _Tz>)
    friend bool operator==(_Ty *lhs, const _Tz & rhs) {
      static_assert(std::is_base_of_v<IUnknown,_Ty>, "Comparison to non IUnknown-derived pointers is forbidden");
#if defined(CATIAR427)
      return equal_to(const_cast<std::remove_const_t<_Ty> *>(lhs), rhs.pointer);
#else  // Pre-R426
      // Solution: 'static_cast' the "XXX_var" to a XXX* pointer to explicit the pointer comparison.
      // Alternatively, you may swap the operands and cast the pointer to what it really is (CATBaseUnknown/IUnknown/...), 
      // but this could change the runtime behavior (CATBaseUnknown::IsEqual vs. pointer comparison)
      return equal_to_symmetric(const_cast<std::remove_const_t<_Ty> *>(lhs), rhs.pointer);
#endif  // Pre-R426
    }
    
#if !defined(CATIAR427)
public:
/** @nodoc "_Ty" is expected to be a "CATBaseUnknown_var"-derived type */
    template <typename _Ty, typename _Tz>
    static constexpr bool var_comp_deleted()
    {
        // In order to maintain compatibility between C++20 builds and pre-C++20 builds, return false to:
        //  1) explicitely disallow constructs that do not compile pre-C++20
        //  2) disallow constructs which behavior are inconsistent pre/post C++20
        // Otherwise, return true to allow the comparison
        using OtherInterface_t = std::remove_const_t<_Tz>;
        if constexpr(!std::is_const_v<_Tz>) {
            // 1) Compile constraints
            if constexpr(std::is_same_v<_Ty, CATBaseUnknown_var> || std::is_same_v<OtherInterface_t, CATBaseUnknown>) {
                return false;
            } else {
                using Interface_t = std::remove_pointer_t<decltype(std::declval<_Ty>().operator->())>;
                constexpr bool bAreRelated = (std::is_base_of_v<Interface_t, OtherInterface_t> || std::is_base_of_v<OtherInterface_t, Interface_t>);
                return bAreRelated;
            }
        } else {  // _Tz is const
            using Interface_t = std::remove_pointer_t<decltype(std::declval<_Ty>().operator->())>;
            
            // 1) Compile constraints
            if(!std::is_base_of_v<OtherInterface_t, Interface_t> && !std::is_base_of_v<OtherInterface_t, CATBaseUnknown>)
                return true;
            
            // 2) Behavior constraints
            return ( !std::is_same_v<Interface_t, OtherInterface_t> || std::is_same_v<Interface_t, CATBaseUnknown>);
        }
    }  // var_comp_deleted
private:
    
/** @nodoc deleted for behavior compatibility with pre-C++20 */
    template <typename _Ty, typename _Tz>
    requires(std::is_base_of_v<CATBaseUnknown_var, _Ty> && var_comp_deleted<_Ty, _Tz>())
    friend bool operator==(const _Ty & lhs, _Tz * rhs) = delete;

/** @nodoc */
    template <typename _Ty, typename _Tz>
    requires(std::is_base_of_v<CATBaseUnknown_var, _Ty> && !var_comp_deleted<_Ty, _Tz>())
    friend bool operator==(const _Ty & lhs, _Tz * rhs) {
        return equal_to(lhs.pointer, const_cast<std::remove_const_t<_Tz> *>(rhs));
    }
#endif  // Pre-R426
    
#endif
};

/**
 * @nodoc
 */
inline CATBaseUnknown_var::CATBaseUnknown_var() : pointer(nullptr) {
}
inline CATBaseUnknown_var::~CATBaseUnknown_var()
{
  if (pointer) pointer->Release();
  pointer = nullptr;
}
// Move semantic support (elide pairs of AddRef/Release, enable thread-safe designs)
inline CATBaseUnknown_var::CATBaseUnknown_var(CATBaseUnknown_var&& other) noexcept
{
  this->pointer = other.pointer;
  other.pointer = nullptr;
}
inline CATBaseUnknown_var::CATBaseUnknown_var(CATBaseUnknown * iPtr): pointer(iPtr)
{
  if (pointer) pointer->AddRef();
}
inline CATBaseUnknown_var::CATBaseUnknown_var(const CATBaseUnknown_var & iRef): pointer(iRef.pointer)
{
  if (pointer) pointer->AddRef();
}
inline CATBaseUnknown_var::operator CATBaseUnknown*() const
{
  return ((CATBaseUnknown *)pointer);
}
inline CATBaseUnknown * CATBaseUnknown_var::operator->() const
{
  if (!pointer) {
      ThrowErrorNullVar();
  }
  return ((CATBaseUnknown *)pointer);
}
inline IUnknown * CATBaseUnknown_var::GetPointer() const
{
  return pointer;
}



/** @nodoc */
CATDeclareHandlerInternal(IUnknown,CATBaseUnknown,ExportedByJS0CORBA)
/** @nodoc */
inline IUnknown_var::IUnknown_var(CATBaseUnknown *base) : CATBaseUnknown_var(base) {
}
/** @nodoc */
typedef IUnknown *IUnknown_ptr;



/** @nodoc */
CATDeclareHandlerInternal(IDispatch,IUnknown,ExportedByJS0CORBA)
/** @nodoc */
inline IDispatch_var::IDispatch_var(CATBaseUnknown *base) : IUnknown_var(base) {
}
/** @nodoc */
typedef IDispatch *IDispatch_ptr;



/**
 * @nodoc
 * for interfaces migration
 */
typedef CATBaseUnknown_var CATInterfaceObject_var;

#endif
