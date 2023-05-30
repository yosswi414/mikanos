/**
 * @file interrupt.hpp
 * @brief 割り込み用のプログラムを集めたファイル
 * @date 2023-05-22
 */

#pragma once

#include <array>
#include <cstdint>

#include "x86_descriptor.hpp"


// #@@range_begin(descriptor_attr_struct)
union InterruptDescriptorAttribute {
    uint16_t data;
    struct {
        // (p.166) mikanOS では常に 0
        uint16_t interrupt_stack_table : 3;
        uint16_t : 5;
        // (p.166) 通常の割り込みは 14 (kInterruptGate)
        DescriptorType type : 4;
        uint16_t : 1;
        uint16_t descriptor_privilege_level : 2;
        // (p.166) 記述子の有効/無効フラグ
        uint16_t present : 1;
    } __attribute__((packed)) bits;
} __attribute__((packed));
// #@@range_end(descriptor_attr_struct)

// #@@range_begin(descriptor_struct)
struct InterruptDescriptor {
    uint16_t offset_low;
    /* (p.166) x86-64 64 ビットモードではセグメンテーション機能は無効化されており
     * 単にメモリ全体の属性を指定する機能となっている
     */
    uint16_t segment_selector;
    InterruptDescriptorAttribute attr;
    uint16_t offset_middle;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed));
// #@@range_end(descriptor_struct)

extern std::array<InterruptDescriptor, 256> idt;

// #@@range_begin(make_idt_attr)
constexpr InterruptDescriptorAttribute MakeIDTAttr(DescriptorType type,
                                                   uint8_t descriptor_privilege_level,
                                                   bool present = true,
                                                   uint8_t interrupt_stack_table = 0) {
    InterruptDescriptorAttribute attr{};
    attr.bits.interrupt_stack_table = interrupt_stack_table;
    attr.bits.type = type;
    attr.bits.descriptor_privilege_level = descriptor_privilege_level;
    attr.bits.present = present;
    return attr;
}
// #@@range_end(make_idt_attr)

void SetIDTEntry(InterruptDescriptor& desc,
                 InterruptDescriptorAttribute attr,
                 uint64_t offset,
                 uint16_t segment_selector);

// #@@range_begin(vector_numbers)
class InterruptVector {
  public:
    enum Number {
        kXHCI = 0x40,
    };
};
// #@@range_end(vector_numbers)

// #@@range_begin(frame_struct)
struct InterruptFrame {
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};
// #@@range_end(frame_struct)

namespace interrupt {
    const uintptr_t lapic_base_default = 0xfee00000;
    class Controller {
      public:
        Controller(uintptr_t lapic_base = lapic_base_default) : lapic_base_{lapic_base} {}
        void NotifyEndOfInterrupt() {
            *end_of_interrupt() = 0;
        }
        const uint8_t GetLAPICID() {
            return *lapic_id() >> 24;
        }
        const uint32_t GetBase() {
            return lapic_base_;
        }

      private:
        uintptr_t lapic_base_;
        uint32_t* lapic_id() { return reinterpret_cast<uint32_t*>(lapic_base_ + 0x20); }
        uint32_t* end_of_interrupt() { return reinterpret_cast<uint32_t*>(lapic_base_ + 0xb0); }
    };

}  // namespace interrupt

/*
CapabilityRegisters* const cap_;

*/