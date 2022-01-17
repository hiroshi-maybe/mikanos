#include "segment.hpp"

#include "asmfunc.h"

namespace {
    std::array<SegmentDescriptor, 3> gdt;
}

void SetCodeSegment(
    SegmentDescriptor& desc,
    DescriptorType type,
    unsigned int descriptor_privilege_level,
    uint32_t base,
    uint32_t limit)
{
    desc.data = 0;

    desc.bits.base_low = base & 0xffffu;
    desc.bits.base_middle = (base >> 16) & 0xffu;
    desc.bits.base_high = (base >> 24) & 0xffu;

    desc.bits.limit_low = limit & 0xffffu;
    desc.bits.limit_high = (limit >> 16) & 0xfu;

    desc.bits.type = type;
    desc.bits.system_segment = 1; // code & data segment
    desc.bits.descriptor_privilege_level = descriptor_privilege_level;
    desc.bits.present = 1;
    desc.bits.available = 0;
    desc.bits.long_mode = 1;
    desc.bits.default_operation_size = 0;
    desc.bits.granularity = 1;
}

void SetDataSegment(
    SegmentDescriptor& desc,
    DescriptorType type,
    unsigned int descriptor_privilege_level,
    uint32_t base,
    uint32_t limit)
{
    SetCodeSegment(desc, type, descriptor_privilege_level, base, limit);
    desc.bits.long_mode = 0;
    desc.bits.default_operation_size = 1;
}

void SetupSegments() {
    // null descriptor
    gdt[0].data = 0;

    // Code segment
    SetCodeSegment(gdt[1], DescriptorType::kExecuteRead, 0, 0, 0xfffff);

    // Data segment
    SetDataSegment(gdt[2], DescriptorType::kReadWrite, 0, 0, 0xfffff);

    // Register gdt for CPU
    LoadGDT(sizeof(gdt) - 1, reinterpret_cast<uintptr_t>(&gdt[0]));
}

void InitializeSegmentation() {
    SetupSegments();

    SetDSAll(kKernelDS); // Point segment registers to the null descriptor
    SetCSSS(kKernelCS, kKernelSS);
}