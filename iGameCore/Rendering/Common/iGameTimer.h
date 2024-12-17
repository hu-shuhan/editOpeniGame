#pragma once

#include "iGameObject.h"
#include <chrono>

IGAME_NAMESPACE_BEGIN
class Timer : public Object {
public:
    I_OBJECT(Timer);
    static Pointer New() { return new Timer; }

    enum class TimeUnit { Microseconds, Milliseconds, Seconds };

    void Reset();

    size_t ElapsedMicroseconds() const;
    size_t ElapsedMilliseconds() const;
    size_t ElapsedSeconds() const;

    size_t Elapsed(TimeUnit unit = TimeUnit::Microseconds) const;

protected:
    Timer();
    ~Timer() override;

    std::chrono::time_point<std::chrono::high_resolution_clock> start;
};
IGAME_NAMESPACE_END
