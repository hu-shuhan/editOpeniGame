/**
 * @class    Timer
 * @brief    Timer类是一个时间工具类，可以用其来记录代码运行时间。
 * @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
 */

#pragma once

#include "iGameObject.h"
#include <chrono>
#include <string>

IGAME_NAMESPACE_BEGIN

class Timer : public Object {
public:
    I_OBJECT(Timer); ///< 声明对象类型的宏。

    /**
     * @brief 创建一个新的 Timer 对象。
     * @return 指向新 Timer 实例的指针。
     */
    static Pointer New() { return new Timer; }

    /**
     * @enum TimeUnit
     * @brief 表示用于测量经过时间的时间单位。
     */
    enum class TimeUnit {
        Microseconds, ///< 微秒。
        Milliseconds, ///< 毫秒。
        Seconds       ///< 秒。
    };

    /**
     * @brief 将计时器重置为当前时间。
     */
    void Reset();

    /**
     * @brief 获取自上次重置以来的经过时间（单位：微秒）。
     * @return 经过的时间，单位为微秒。
     */
    size_t ElapsedMicroseconds() const;

    /**
     * @brief 获取自上次重置以来的经过时间（单位：毫秒）。
     * @return 经过的时间，单位为毫秒。
     */
    size_t ElapsedMilliseconds() const;

    /**
     * @brief 获取自上次重置以来的经过时间（单位：秒）。
     * @return 经过的时间，单位为秒。
     */
    size_t ElapsedSeconds() const;

    /**
     * @brief 获取自上次重置以来的经过时间，以指定的单位返回。
     * @param unit 要返回经过时间的时间单位（默认为微秒）。
     * @return 以指定单位表示的经过时间。
     */
    size_t Elapsed(TimeUnit unit = TimeUnit::Microseconds) const;

protected:
    /**
     * @brief 构造一个 Timer 对象。
     */
    Timer();

    /**
     * @brief 销毁 Timer 对象。
     */
    ~Timer() override;

    /**
     * @brief 存储计时器的起始时间点。
     */
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
};

class AutoTimer {
public:
    AutoTimer(const std::string& timerName,
              Timer::TimeUnit unit = Timer::TimeUnit::Milliseconds);
    AutoTimer(Timer::TimeUnit unit = Timer::TimeUnit::Milliseconds);
    AutoTimer(int name, Timer::TimeUnit unit = Timer::TimeUnit::Milliseconds);
    ~AutoTimer();

private:
    std::string m_TimerName;
    Timer::Pointer m_Timer;
    Timer::TimeUnit m_Unit;
    static int GetAutoAddInt();
};

class AutoBlockTimer {
public:
    AutoBlockTimer(int name,
                   Timer::TimeUnit unit = Timer::TimeUnit::Milliseconds);
    ~AutoBlockTimer();
    void OneBlock(int name);

private:
    std::string m_TimerName;
    Timer::Pointer m_Timer;
    Timer::TimeUnit m_Unit;
    void SetTimeName(int name);
    void OutPut();
};

#define ___AT__(name) iGame::AutoTimer AT##name(##name)
#define __AT__(line) ___AT__(line)
#define ___ABT_NEW__(name) iGame::AutoBlockTimer ABT_NEW(##name)
#define __ABT_NEW__(line) ___ABT_NEW__(line)
#define ___ABT__(name) ABT_NEW.OneBlock(##name)
#define __ABT__(line) ___ABT__(line)

//#define AT_DEBUG
#ifdef AT_DEBUG
    #define _AT_ __AT__(__LINE__)
    #define _ABT_NEW_ __ABT_NEW__(__LINE__)
    #define _ABT_ __ABT__(__LINE__)
#else
    #define _AT_
    #define _ABT_NEW_
    #define _ABT_
#endif // DEBUG
    


IGAME_NAMESPACE_END