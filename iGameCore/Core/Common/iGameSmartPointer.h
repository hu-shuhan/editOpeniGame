#ifndef iGameSmartPointer_h
#define iGameSmartPointer_h

#include <iostream>
#include <utility>
#include "iGameMacro.h"

IGAME_NAMESPACE_BEGIN
template <typename TObjectType>
class SmartPointer
{
public:
    using ObjectType = TObjectType;

    template <typename T>
    using EnableIfConvertible = typename std::enable_if<std::is_convertible_v<T*, TObjectType*>>;

    constexpr SmartPointer() noexcept = default;
    constexpr SmartPointer(std::nullptr_t) noexcept {}

    template <typename T, typename = typename EnableIfConvertible<T>::type>
    SmartPointer(const SmartPointer<T>& p) noexcept
        : m_Pointer(p.m_Pointer)
    {
        this->Register();
    }

    // Copy constructor
    SmartPointer(const SmartPointer& p) noexcept 
        : m_Pointer(p.m_Pointer) 
    {
        this->Register();
    }

    // Move constructor
    SmartPointer(SmartPointer<ObjectType>&& p) noexcept
        : m_Pointer(p.m_Pointer)
    {
        p.m_Pointer = nullptr;
    }

    // move constructor with implicit conversion of pointer type
    template <typename T, typename = typename EnableIfConvertible<T>::type>
    SmartPointer(SmartPointer<T>&& p) noexcept
        : m_Pointer(p.m_Pointer)
    {
        p.m_Pointer = nullptr;
    }

    // Constructor to pointer p
    SmartPointer(ObjectType* p) noexcept
        : m_Pointer(p)
    {
        this->Register();
    }

    // Destructor
    ~SmartPointer() { this->UnRegister(); }

    // Overload operator
    ObjectType* operator->() const noexcept { return m_Pointer; }

    ObjectType& operator*() const noexcept { return *m_Pointer; }

    explicit operator bool() const noexcept { return m_Pointer != nullptr; }

    // Return pointer to object
    operator ObjectType* () const noexcept { return m_Pointer; }

    bool IsNotNull() const noexcept
    {
        return m_Pointer != nullptr;
    }

    bool IsNull() const noexcept
    {
        return m_Pointer == nullptr;
    }

    // Access function to pointer.
    ObjectType* GetPointer() const noexcept
    {
        return m_Pointer;
    }

    ObjectType* get() const noexcept
    {
        return m_Pointer;
    }


    SmartPointer& operator=(SmartPointer r) noexcept
    {
        this->Swap(r);
        return *this;
    }

    SmartPointer& operator=(std::nullptr_t) noexcept
    {
        this->UnRegister();
        this->m_Pointer = nullptr;
        return *this;
    }

    void Swap(SmartPointer& other) noexcept
    {
        ObjectType* tmp = this->m_Pointer;
        this->m_Pointer = other.m_Pointer;
        other.m_Pointer = tmp;
    }

private:
    // The pointer to the object referred to by this smart pointer.
    ObjectType* m_Pointer{ nullptr };

    template <typename T>
    friend class SmartPointer;

    void Register() noexcept
    {
        if (m_Pointer)
        {
            m_Pointer->Register();
        }
    }

    void UnRegister() noexcept
    {
        if (m_Pointer)
        {
            m_Pointer->UnRegister();
        }
    }
};

template <typename TTarget, typename TSource>
SmartPointer<TTarget> DynamicCast(const SmartPointer<TSource>& source)
{
    TTarget* target = dynamic_cast<TTarget*>(source.GetPointer());
    return SmartPointer<TTarget>(target);
}

template <typename TTarget, typename TSource>
TTarget* DynamicCast(TSource* source)
{
    return dynamic_cast<TTarget*>(source);
}

IGAME_NAMESPACE_END
#endif