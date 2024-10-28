#ifndef iGameObject_h
#define iGameObject_h

#include <cassert>
#include <functional>
#include <memory>
#include <cstring>

#include <array>
#include <map>
#include <queue>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cfloat>

#include "iGameAbstractObject.h"
#include "iGameTimeStamp.h"

IGAME_NAMESPACE_BEGIN
class Command;
class ObserverInternal;

class Object : public AbstractObject {
public:
    I_OBJECT(Object);
    static Pointer New() { return new Object; }

    using CommandPointer = SmartPointer<Command>;

    // Update TimeStamp
    void Modified();

    // Get object's TimeStamp
    TimeStamp& GetMTime() { return m_MTime; }

    // Set/Get object's name
    void SetName(const std::string& name) { this->m_Name = name; }

    const std::string& GetName() const { return this->m_Name; }


    unsigned long AddObserver(unsigned long event, CommandPointer cmd,
                              float priority = 0.0f);
    unsigned long AddObserver(const char* event, CommandPointer cmd,
                              float priority = 0.0f);

    unsigned long AddObserver(unsigned long event,
                              std::function<void()> callback,
                              float priority = 0.0f);
    unsigned long
    AddObserver(unsigned long event,
                std::function<void(Object*, unsigned long, void*)> callback,
                float priority = 0.0f);

    template<class T>
    unsigned long AddObserver(unsigned long event, void (T::*callback)(),
                              T* instance, float priority = 0.0f) {
        auto func = std::bind(callback, instance);
        return this->AddObserver(event, func, priority);
    }
    template<class T>
    unsigned long AddObserver(unsigned long event,
                              void (T::*callback)(Object*, unsigned long,
                                                  void*),
                              T* instance, float priority = 0.0f) {
        auto func = std::bind(callback, instance);
        return this->AddObserver(event, func, priority);
    }

    void RemoveObserver(unsigned long tag);
    void RemoveObservers(unsigned long event);
    void RemoveObservers(const char* event);
    void RemoveAllObservers();
    bool HasObserver(unsigned long event);
    bool HasObserver(const char* event);

    CommandPointer GetCommand(unsigned long tag);
    void RemoveObserver(CommandPointer cmd);
    void RemoveObservers(unsigned long event, CommandPointer cmd);
    void RemoveObservers(const char* event, CommandPointer cmd);
    bool HasObserver(unsigned long event, CommandPointer cmd);
    bool HasObserver(const char* event, CommandPointer cmd);

    bool InvokeEvent(unsigned long event, void* callData);
    bool InvokeEvent(const char* event, void* callData);
    bool InvokeEvent(unsigned long event) {
        return this->InvokeEvent(event, nullptr);
    }
    bool InvokeEvent(const char* event) {
        return this->InvokeEvent(event, nullptr);
    }

protected:
    Object() { /*this->Modified();*/ }
    ~Object() override {
        if (m_Internal) { delete m_Internal; }
    };

    TimeStamp m_MTime{};
    ObserverInternal* m_Internal{nullptr};
    std::string m_Name{};
};
IGAME_NAMESPACE_END
#endif