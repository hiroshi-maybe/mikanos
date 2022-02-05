#include "timer.hpp"

#include "interrupt.hpp"

namespace {

const uint32_t kCountMax = 0xffffffffu;
volatile uint32_t& lvt_timer = *reinterpret_cast<uint32_t*>(0xfee00320);
volatile uint32_t& initial_count = *reinterpret_cast<uint32_t*>(0xfee00380);
volatile uint32_t& current_count = *reinterpret_cast<uint32_t*>(0xfee00390);
volatile uint32_t& divide_config = *reinterpret_cast<uint32_t*>(0xfee003e0);

const uint32_t TIMER_FREQUENCY_RATE_PER_CPU_CLOCK_1 = 0b1011;
const uint32_t INTERRUPT_LVT_TIMER_REG = 0b010 << 16;

}

void InitializeLAPICTimer() {
    divide_config = TIMER_FREQUENCY_RATE_PER_CPU_CLOCK_1; // frequency rate = 1
    lvt_timer = INTERRUPT_LVT_TIMER_REG | InterruptVector::kLAPICTimer; // not-masked, periodic
    initial_count = kCountMax;
}

void StartLAPICTimer() {
    initial_count = kCountMax;
}
uint32_t LAPICTimerElapsed() {
    return kCountMax - current_count;
}

void StopLAPICTimer() {
    initial_count = 0;
}
