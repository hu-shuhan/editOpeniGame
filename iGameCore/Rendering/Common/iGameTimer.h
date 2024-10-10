#pragma once

#include "iGameObject.h"
#include <chrono>

IGAME_NAMESPACE_BEGIN
class Timer : public Object {
public:
    I_OBJECT(Timer);
    static Pointer New() { return new Timer; }

public:
    enum class TimeUnit { Microseconds, Milliseconds, Seconds };

    void Reset() { start = std::chrono::high_resolution_clock::now(); }

    size_t ElapsedMicroseconds() const {
        return Elapsed(TimeUnit::Microseconds);
    }

    size_t ElapsedMilliseconds() const {
        return Elapsed(TimeUnit::Milliseconds);
    }

    size_t ElapsedSeconds() const { return Elapsed(TimeUnit::Seconds); }

    size_t Elapsed(TimeUnit unit = TimeUnit::Microseconds) const {
        auto now = std::chrono::high_resolution_clock::now();
        switch (unit) {
            case TimeUnit::Microseconds:
                return std::chrono::duration_cast<std::chrono::microseconds>(
                               now - start)
                        .count();
            case TimeUnit::Milliseconds:
                return std::chrono::duration_cast<std::chrono::milliseconds>(
                               now - start)
                        .count();
            case TimeUnit::Seconds:
                return std::chrono::duration_cast<std::chrono::seconds>(now -
                                                                        start)
                        .count();
        }
        return 0;
    }

protected:
    Timer() { Reset(); }
    ~Timer() override = default;

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
};
IGAME_NAMESPACE_END
