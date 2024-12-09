#pragma once

#include "iGameObject.h"
#include <queue>
#include <unordered_map>
#include <unordered_set>

IGAME_NAMESPACE_BEGIN

template<typename ObjectType>
class HandlePool : public Object {
public:
    I_OBJECT(HandlePool);
    static Pointer New() { return new HandlePool; }

    using HandleType = IGuint;
    using MapType = std::unordered_map<HandleType, ObjectType>;
    using Iterator = typename MapType::iterator;
    using ConstIterator = typename MapType::const_iterator;

    HandleType AllocateObject(const ObjectType& object) {
        HandleType handle = m_FreeHandles.empty() ? m_CurrentHandle++
                                                  : m_FreeHandles.front();
        if (!m_FreeHandles.empty()) { m_FreeHandles.pop(); }
        m_ActiveHandles.insert(handle);
        m_HandleToObject[handle] = std::move(object);

        this->Modified();
        return handle;
    }

    ObjectType* GetObject(HandleType handle) {
        auto it = m_HandleToObject.find(handle);
        if (it != m_HandleToObject.end()) { return &it->second; }
        return nullptr;
    }

    void ReleaseHandle(HandleType handle) {
        if (m_ActiveHandles.erase(handle)) {
            m_FreeHandles.push(handle);
            m_HandleToObject.erase(handle);
            this->Modified();
        }
    }

    bool CheckHandle(HandleType handle) const {
        return m_ActiveHandles.find(handle) != m_ActiveHandles.end();
    }

    void Clear() {
        m_FreeHandles = std::queue<HandleType>(); // Reset the queue
        m_ActiveHandles.clear();                  // Clear the set
        m_HandleToObject.clear();                 // Clear the map
        m_CurrentHandle = 1; // Optionally reset handle counter if needed
        this->Modified();
    }

    Iterator begin() { return m_HandleToObject.begin(); }
    Iterator end() { return m_HandleToObject.end(); }
    ConstIterator begin() const { return m_HandleToObject.begin(); }
    ConstIterator end() const { return m_HandleToObject.end(); }

protected:
    HandlePool() : m_CurrentHandle(1) {}
    ~HandlePool() override = default;

    HandleType m_CurrentHandle;
    std::queue<HandleType> m_FreeHandles;
    std::unordered_set<HandleType> m_ActiveHandles;
    std::unordered_map<HandleType, ObjectType> m_HandleToObject;
};

IGAME_NAMESPACE_END
