#pragma once

#include <cstdint>

#include "logger.hpp"

void InitializeLAPICTimer();
void StartLAPICTimer();
uint32_t LAPICTimerElapsed();
void StopLAPICTimer();

template<typename Func, typename Logger>
void measure_with_LAPICTimer(const char* tag, Logger logger, Func execute) {
    StartLAPICTimer();
    execute();
    auto elapsed = LAPICTimerElapsed();
    StopLAPICTimer();
    logger("%s: elapsed = %f02\n", tag, (double)elapsed/1e6);
}

class TimerManager {
public:
    void Tick();
    unsigned long CurrentTick() const { return tick_; }
private:
    volatile unsigned long tick_{0};
};

extern TimerManager* timer_manager;

void LAPICTimerOnInterrupt();
