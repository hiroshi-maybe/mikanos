#include "asmfunc.h"
#include "interrupt.hpp"
#include "logger.hpp"
#include "segment.hpp"

const uint32_t END_OF_INTERRUPT_REG_ADDR = 0xfee000b0;

std::array<InterruptDescriptor, 256> idt;

void SetIDTEntry(
    InterruptDescriptor& desc, InterruptDescriptorAttribute attr,
    uint64_t offset, uint16_t segment_selector)
{
    desc.attr = attr;
    desc.offset_low = offset & 0xffffu;
    desc.offset_middle = (offset >> 16) & 0xffffu;
    desc.offset_high = offset >> 32;
    desc.segment_selector = segment_selector;
}

void NotifyEndOfInterrupt() {
    volatile auto end_of_interrupt = reinterpret_cast<uint32_t*>(END_OF_INTERRUPT_REG_ADDR);
    *end_of_interrupt = 0;
}

namespace {
    std::deque<Message>* msg_queue;

    __attribute__((interrupt))
    void IntHandlerXHCI(InterruptFrame* frame) {
        Log(kDebug, "Interrupt happened\n");
        msg_queue->push_back(Message{Message::kInterruptXHCI});
        NotifyEndOfInterrupt();
    }
}

void InitializeInterrupt(std::deque<Message>* msg_queue) {
    ::msg_queue = msg_queue;

    SetIDTEntry(idt[InterruptVector::kXHCI], MakeIDTAttr(DescriptorType::kInterruptGate, 0),
                reinterpret_cast<uint64_t>(IntHandlerXHCI), kKernelCS);
    LoadIDT(sizeof(idt) - 1, reinterpret_cast<uintptr_t>(&idt[0]));
}