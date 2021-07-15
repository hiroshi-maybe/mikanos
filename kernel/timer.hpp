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
