#pragma once

#pragma once

#include "iGameObject.h"
#include <chrono>

IGAME_NAMESPACE_BEGIN
class iGameTimer : public Object {
public:
    enum class TimeUnit { Microseconds, Milliseconds, Seconds };

    iGameTimer() { reset(); }

    void reset() { start = std::chrono::high_resolution_clock::now(); }

    size_t elapsedMicroseconds() const {
        return elapsed(TimeUnit::Microseconds);
    }
    size_t elapsedMilliseconds() const {
        return elapsed(TimeUnit::Milliseconds);
    }
    size_t elapsedSeconds() const { return elapsed(TimeUnit::Seconds); }

    size_t elapsed(TimeUnit unit = TimeUnit::Microseconds) const {
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

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
};
IGAME_NAMESPACE_END
