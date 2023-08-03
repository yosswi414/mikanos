#include "timer.hpp"

namespace {
    // LVT timer
    const uint32_t LAPIC_TIMER_ADDR_LVT_TIMER = 0xfee00320;
    // initial count
    const uint32_t LAPIC_TIMER_ADDR_INIT_COUNT = 0xfee00380;
    // current count
    const uint32_t LAPIC_TIMER_ADDR_CURRENT_COUNT = 0xfee00390;
    // divide configuration
    const uint32_t LAPIC_TIMER_ADDR_DIV_CONFIG = 0xfee003e0;

    const uint32_t kCountMax = 0xffffffffu;
    volatile uint32_t& lvt_timer = *reinterpret_cast<uint32_t*>(LAPIC_TIMER_ADDR_LVT_TIMER);
    volatile uint32_t& initial_count = *reinterpret_cast<uint32_t*>(LAPIC_TIMER_ADDR_INIT_COUNT);
    volatile uint32_t& current_count = *reinterpret_cast<uint32_t*>(LAPIC_TIMER_ADDR_CURRENT_COUNT);
    volatile uint32_t& divide_config = *reinterpret_cast<uint32_t*>(LAPIC_TIMER_ADDR_DIV_CONFIG);
}  // namespace

void InitializeLAPICTimer() {
    // div config bit 3, 1, 0 represents the division ratio (p.227)
    divide_config = 0b1011;          // divide 1:1
    /**
     * LVT Timer register field (p.227)
     * bits     | field             | description
     * ---------|-------------------|------------------------
     * 0:7      | Vector            | interrupt vector number
     * 12       | Delivery Status   | interrupt delivery status (0=empty, 1=waiting for delivery)
     * 16       | Mask              | interrupt mask (0=send interrupt, 1=don't interrupt)
     * 17:18    | Timer Mode        | timer function mode (0=oneshot, 1=periodic)
     */
    lvt_timer = (0b001 << 16) | 32;  // masked, oneshot
}

void StartLAPICTimer() {
    // write value to start with;
    // the value will be copied to current_count and then it starts to count down to 0.
    // if current_count reaches 0, it sends interrupt with vector number set at 0:7 of 
    // LVT timer register. (p.228)
    initial_count = kCountMax;
}

uint32_t LAPICTimerElapsed() {
    return kCountMax - current_count;
}

void StopLAPICTimer() {
    // write zero to stop the timer
    initial_count = 0;
}