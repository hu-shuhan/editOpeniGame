#include "iGameHandlePool.h"

IGAME_NAMESPACE_BEGIN

template<typename ObjectType>
HandlePool<ObjectType>::HandlePool() : m_CurrentHandle(1) {}

template<typename ObjectType>
HandlePool<ObjectType>::~HandlePool() = default;

template<typename ObjectType>
typename HandlePool<ObjectType>::HandleType
HandlePool<ObjectType>::AllocateObject(const ObjectType& object) {
    HandleType handle =
            m_FreeHandles.empty() ? m_CurrentHandle++ : m_FreeHandles.front();
    if (!m_FreeHandles.empty()) { m_FreeHandles.pop(); }
    m_ActiveHandles.insert(handle);
    m_HandleToObject[handle] = std::move(object);

    this->Modified();
    return handle;
}

template<typename ObjectType>
void HandlePool<ObjectType>::ReleaseHandle(HandleType handle) {
    if (m_ActiveHandles.erase(handle)) {
        m_FreeHandles.push(handle);
        m_HandleToObject.erase(handle);
        this->Modified();
    }
}

template<typename ObjectType>
bool HandlePool<ObjectType>::CheckHandle(HandleType handle) const {
    return m_ActiveHandles.find(handle) != m_ActiveHandles.end();
}

template<typename ObjectType>
HandlePool<ObjectType>::ReturnType
HandlePool<ObjectType>::GetObjectByHandle(HandleType handle) {
    auto it = m_HandleToObject.find(handle);
    if (it != m_HandleToObject.end()) {
        if constexpr (is_smart_pointer<ObjectType>::value) {
            return it->second;
        } else {
            return &it->second;
        }
    }
    return nullptr;
}

template<typename ObjectType>
typename HandlePool<ObjectType>::HandleType
HandlePool<ObjectType>::GetObjectCount() const {
    return m_HandleToObject.size();
}

template<typename ObjectType>
void HandlePool<ObjectType>::Clear() {
    m_FreeHandles = std::queue<HandleType>(); // Reset the queue
    m_ActiveHandles.clear();                  // Clear the set
    m_HandleToObject.clear();                 // Clear the map
    m_CurrentHandle = 1; // Optionally reset handle counter if needed
    this->Modified();
}

template<typename ObjectType>
typename HandlePool<ObjectType>::Iterator HandlePool<ObjectType>::Begin() {
    return m_HandleToObject.begin();
}

template<typename ObjectType>
typename HandlePool<ObjectType>::Iterator HandlePool<ObjectType>::End() {
    return m_HandleToObject.end();
}

template<typename ObjectType>
typename HandlePool<ObjectType>::ConstIterator
HandlePool<ObjectType>::Begin() const {
    return m_HandleToObject.begin();
}

template<typename ObjectType>
typename HandlePool<ObjectType>::ConstIterator
HandlePool<ObjectType>::End() const {
    return m_HandleToObject.end();
}

//template<typename ObjectType>
//HandlePool<ObjectType>::Iterator HandlePool<ObjectType>::begin() {
//    return m_HandleToObject.begin();
//}
//
//template<typename ObjectType>
//HandlePool<ObjectType>::Iterator HandlePool<ObjectType>::end() {
//    return m_HandleToObject.end();
//}
//
//template<typename ObjectType>
//typename HandlePool<ObjectType>::ConstIterator
//HandlePool<ObjectType>::begin() const {
//    return m_HandleToObject.begin();
//}
//
//template<typename ObjectType>
//typename HandlePool<ObjectType>::ConstIterator
//HandlePool<ObjectType>::end() const {
//    return m_HandleToObject.end();
//}

IGAME_NAMESPACE_END
