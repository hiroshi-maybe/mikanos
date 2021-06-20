#include "interrupt.hpp"

const uint32_t END_OF_INTERRUPT_REG_ADDR = 0xfee000b0;

void NotifyEndOfInterrupt() {
    volatile auto end_of_interrupt = reinterpret_cast<uint32_t*>(END_OF_INTERRUPT_REG_ADDR);
    *end_of_interrupt = 0;
}
