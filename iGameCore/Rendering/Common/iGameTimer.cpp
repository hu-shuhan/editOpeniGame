#include "iGameTimer.h"
#include <iostream>
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

AutoTimer::AutoTimer(const std::string& timerName, Timer::TimeUnit unit)
    : m_TimerName(timerName), m_Unit(unit) {
    m_Timer = Timer::New();
}

AutoTimer::AutoTimer(Timer::TimeUnit unit) : m_Unit(unit) {
    m_TimerName = std::string("Timer") + std::to_string(GetAutoAddInt());
    m_Timer = Timer::New();
}

AutoTimer::AutoTimer(int name, Timer::TimeUnit unit) : m_Unit(unit) {
    m_TimerName = std::string("Timer") + std::to_string(name);
    m_Timer = Timer::New();
}

AutoTimer::~AutoTimer() {
    auto elsp = m_Timer->Elapsed(m_Unit);
    std::cout << m_TimerName << ":" << elsp;
    switch (m_Unit) {
        case Timer::TimeUnit::Microseconds:
            std::cout << " us";
            break;
        case Timer::TimeUnit::Milliseconds:
            std::cout << " ms";
            break;
        case Timer::TimeUnit::Seconds:
            std::cout << " s";
            break;
    }
    std::cout << std::endl;
}

int AutoTimer::GetAutoAddInt() {
    static int AutoAddInt = -1;
    AutoAddInt++;
    return AutoAddInt;
}

IGAME_NAMESPACE_END