
#pragma once

#include <cstdint>
#include <array>

#include "x86_descriptor.hpp"

union SegmentDescriptor {
    uint64_t data;
    struct{
        uint64_t limit_low : 16;                    // limit: size(byte) of the segment - 1 (ignored in 64-bit mode)
        uint64_t base_low : 16;                     // base: start address of the segment (ignored in 64-bit mode)
        uint64_t base_middle : 8;
        DescriptorType type : 4;                    // descriptor type
        uint64_t system_segment : 1;                // 1: code or data segment
        uint64_t descriptor_privilege_level : 2;    // privilege level
        uint64_t present : 1;                       // 1: enable the descriptor
        uint64_t limit_high : 4;
        uint64_t available : 1;                     // single bit which OS can handle
        uint64_t long_mode : 1;                     // 1: code segment for 64-bit
        uint64_t default_operation_size : 1;        // should be 0 if long_mode == 1
        uint64_t granularity : 1;                   // 1: interpret limit as the number of 4KiB blocks (ignored in 64-bit mode)
        uint64_t base_high : 8;
    } __attribute__((packed)) bits;
} __attribute__((packed));

void SetCodeSegment(SegmentDescriptor& desc,
                    DescriptorType type,
                    unsigned int descriptor_privilege_level,
                    uint32_t base,
                    uint32_t limit);

void SetDataSegment(SegmentDescriptor& desc,
                    DescriptorType type,
                    unsigned int descriptor_privilege_level,
                    uint32_t base,
                    uint32_t limit);

// kKernelCS: also referred in call of SetIDTEntry()
const uint16_t kKernelCS = 1 << 3;  // points to gdt[1]: code segment
const uint16_t kKernelSS = 2 << 3;  // points to gdt[2]: data segment
const uint16_t kKernelDS = 0;       // point to null descriptor gdt[0]; DS and ES won't be used in x86-64 64-bit mode
                                    // and FS, GS too unless explicitly used by programmer (p.193)

void SetupSegments();
void InitializeSegmentation();
