//
// Created by Sumzeek on 12/9/2024.
//

#include "iGameTimer.h"

IGAME_NAMESPACE_BEGIN

Timer::Timer() { Reset(); }

Timer::~Timer() {}

void Timer::Reset() { start = std::chrono::high_resolution_clock::now(); }

size_t Timer::ElapsedMicroseconds() const {
    return Elapsed(TimeUnit::Microseconds);
}

size_t Timer::ElapsedMilliseconds() const {
    return Elapsed(TimeUnit::Milliseconds);
}

size_t Timer::ElapsedSeconds() const { return Elapsed(TimeUnit::Seconds); }

size_t Timer::Elapsed(TimeUnit unit) const {
    auto now = std::chrono::high_resolution_clock::now();
    switch (unit) {
        case TimeUnit::Microseconds:
            return std::chrono::duration_cast<std::chrono::microseconds>(now -
                                                                         start)
                    .count();
        case TimeUnit::Milliseconds:
            return std::chrono::duration_cast<std::chrono::milliseconds>(now -
                                                                         start)
                    .count();
        case TimeUnit::Seconds:
            return std::chrono::duration_cast<std::chrono::seconds>(now - start)
                    .count();
    }
    return 0;
}

IGAME_NAMESPACE_END