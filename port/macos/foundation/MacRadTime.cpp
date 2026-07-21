#include <radtime.hpp>

#include <atomic>
#include <chrono>
#include <ctime>

namespace
{
using Clock = std::chrono::steady_clock;
const Clock::time_point kEpoch = Clock::now();
std::atomic_uint sInitializeCount { 0 };

unsigned long long ElapsedMicroseconds()
{
    return static_cast<unsigned long long>(std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - kEpoch).count());
}
}

void radTimeInitialize(void) { ++sInitializeCount; }
void radTimeTerminate(void) { if (sInitializeCount != 0) --sInitializeCount; }
unsigned int radTimeGetMicroseconds(void) { return static_cast<unsigned int>(ElapsedMicroseconds()); }
radTime64 radTimeGetMicroseconds64(void) { return static_cast<radTime64>(ElapsedMicroseconds()); }
unsigned int radTimeGetMilliseconds(void) { return static_cast<unsigned int>(ElapsedMicroseconds() / 1000ULL); }
unsigned int radTimeGetSeconds(void) { return static_cast<unsigned int>(ElapsedMicroseconds() / 1000000ULL); }

void radTimeGetDate(radDate* date)
{
    if (date == nullptr) return;
    const std::time_t now = std::time(nullptr);
    std::tm local {};
    localtime_r(&now, &local);
    date->m_Year = static_cast<unsigned short>(local.tm_year + 1900);
    date->m_Month = static_cast<unsigned char>(local.tm_mon + 1);
    date->m_Day = static_cast<unsigned char>(local.tm_mday);
    date->m_Hour = static_cast<unsigned char>(local.tm_hour);
    date->m_Minute = static_cast<unsigned char>(local.tm_min);
    date->m_Second = static_cast<unsigned char>(local.tm_sec);
}
