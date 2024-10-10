#pragma once

#include "iGameObject.h"
#include <queue>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

IGAME_NAMESPACE_BEGIN

template<typename ObjectType>
class HandlePool : public Object {
public:
    I_OBJECT(HandlePool);
    static Pointer New() { return new HandlePool; }

public:
    using HandleType = IGuint;
    using MapType = std::unordered_map<HandleType, ObjectType>;
    using Iterator = typename MapType::iterator;
    using ConstIterator = typename MapType::const_iterator;

public:
    HandleType AllocateObject(const ObjectType& object) {
        HandleType handle;
        if (!freeHandles.empty()) {
            handle = freeHandles.front();
            freeHandles.pop();
        } else {
            handle = currentHandle++;
        }
        activeHandles.insert(handle);
        handleToObject[handle] = object;
        return handle;
    }

    ObjectType& GetObject(HandleType handle) {
        CheckHandle(handle);
        return handleToObject.at(handle);
    }

    void ReleaseHandle(HandleType handle) {
        if (activeHandles.erase(handle)) {
            freeHandles.push(handle);
            handleToObject.erase(handle);
        }
    }

    void CheckHandle(HandleType handle) const {
        if (activeHandles.find(handle) == activeHandles.end()) {
            throw std::runtime_error("Handle is not valid!");
        }
    }

    void Clear() {
        freeHandles = std::queue<HandleType>(); // Reset the queue
        activeHandles.clear();                  // Clear the set
        handleToObject.clear();                 // Clear the map
        currentHandle = 1; // Optionally reset handle counter if needed
    }

    Iterator begin() { return handleToObject.begin(); }
    Iterator end() { return handleToObject.end(); }
    ConstIterator begin() const { return handleToObject.begin(); }
    ConstIterator end() const { return handleToObject.end(); }

private:
    HandlePool() : currentHandle(1) {}
    ~HandlePool() override = default;

    HandleType currentHandle;
    std::queue<HandleType> freeHandles;
    std::unordered_set<HandleType> activeHandles;
    std::unordered_map<HandleType, ObjectType> handleToObject;
};

IGAME_NAMESPACE_END
