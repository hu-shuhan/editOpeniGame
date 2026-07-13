#pragma once

#include <chrono>
#include "iGameGP_Macros.h"

namespace gpmesh {

struct CPUTimer
{
    CPUTimer()
    {
    }
    ~CPUTimer()
    {
    }
    void start()
    {
        m_start = std::chrono::high_resolution_clock::now();
    }
    void stop()
    {
        m_stop = std::chrono::high_resolution_clock::now();
    }
    float elapsed_millis()
    {
        return std::chrono::duration<float, std::milli>(m_stop - m_start)
            .count();
    }

   private:
    std::chrono::high_resolution_clock::time_point m_start;
    std::chrono::high_resolution_clock::time_point m_stop;
};
}    