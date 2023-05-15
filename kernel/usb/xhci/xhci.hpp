#pragma once

#include "error.hpp"
#include "logger.hpp"

#define CAPLEN *(volatile uint8_t*)(base + 0x00)
#define HCIVERSION *(volatile uint16_t*)(base + 0x02)
#define RTSOFF *(volatile uint32_t*)(base + 0x18)
#define USBCMD *(volatile uint32_t*)(opreg_base + 0x00)
#define USBSTS *(volatile uint32_t*)(opreg_base + 0x04)
#define HCSPARAMS1 *(volatile uint32_t*)(base + 0x4)
#define DCBAAP *(volatile uint64_t*)(opreg_base + 0x30)
#define CONFIG *(volatile uint32_t*)(opreg_base + 0x38)
#define CRCR *(volatile uint64_t*)(opreg_base + 0x18)
#define ERSTSZ(idx) *(volatile uint32_t*)(rnreg_base + 0x28 + 32 * idx)
#define ERSTBA(idx) *(volatile uint64_t*)(rnreg_base + 0x30 + 32 * idx)

namespace usb {
    namespace xhci {
        struct Controller {
            uint64_t base;
            uint64_t opreg_base;
            uint64_t rnreg_base;
            const size_t kDeviceSize = 8;
            Controller(const uint64_t& addr) : base(addr) {
                opreg_base = base + CAPLEN;
                rnreg_base = base + (RTSOFF & 0xffffffe0u);
            }

            Error Initialize() {
                if (base == 0) return Error(Error::kEmpty, NULL, 0);
                Log(kDebug, "xhci.hpp build no:017\n");
                Log(kDebug, "   base: 0x%016lx\n", base);
                Log(kDebug, "op base: 0x%016lx\n", opreg_base);
                Log(kDebug, "rn base: 0x%016lx\n", rnreg_base);
                Log(kDebug, "CAPLEN:CAPLENGTH : 0x%02lx\n", CAPLEN);
                Log(kDebug, "CAPLEN:HCIVERSION: 0x%04lx\n", HCIVERSION);
                Log(kDebug, "USBCMD: 0x%08lx\n", USBCMD & 0xffffffffu);
                Log(kDebug, "USBSTS: 0x%08lx\n", USBSTS & 0xffffffffu);

                // while (1) __asm__("hlt");

                bool HCError = USBSTS & (1u << 12);
                bool CNReady = USBSTS & (1u << 11);
                bool HCHalted = USBSTS & 1u;

                Log(kDebug, "HCHalted: %s\n", HCHalted ? "True" : "False");
                Log(kDebug, "HCError:  %s\n", HCError ? "True" : "False");
                Log(kDebug, "CNReady:  %s\n", CNReady ? "True" : "False");

                if (!HCHalted) return Error(Error::kInvalidPhase, NULL, 0);
                USBCMD |= (1u << 1);

                volatile uint32_t cnt = 1e8;
                // while (cnt > 0 && (USBCMD & (1ull << 1))) { --cnt; }
                while (cnt > 0) {
                    --cnt;
                    volatile uint32_t tmp = USBCMD;
                    if (!(tmp & (1u << 1))) {
                        Log(kDebug, "USBCMD OK: 0x%08lx\n", tmp & 0xffffffffu);
                        break;
                    }
                }
                if (cnt == 0)
                    Log(kDebug, "USBCMD timeout\n");
                else
                    cnt = 1e8;
                while (cnt > 0) {
                    --cnt;
                    volatile uint32_t tmp = USBSTS;
                    if (!(tmp & (1u << 11))) {
                        Log(kDebug, "USBSTS OK: 0x%08lx\n", tmp & 0xffffffffu);
                        break;
                    }
                }
                if (cnt == 0)
                    Log(kDebug, "USBSTS timeout\n");
                else
                    Log(kDebug, "Host Controller Reset\n");

                Log(kDebug, "USBCMD: 0x%08lx\n", USBCMD & 0xffffffffu);
                Log(kDebug, "USBSTS: 0x%08lx\n", USBSTS & 0xffffffffu);

                volatile uint8_t maxSlots = (HCSPARAMS1 & 0xff);

                Log(kDebug, "MaxSlots: %d\n", maxSlots);

                volatile const uint8_t NUM_DEVICE_SLOT = 7;

                CONFIG = (CONFIG & 0xffffff00u) | NUM_DEVICE_SLOT;

                union TRB {
                    std::array<uint32_t, 4> data{};
                    struct {
                        uint64_t parameter;
                        uint32_t status;
                        uint32_t cycle_bit : 1;
                        uint32_t evaluate_next_trb : 1;
                        uint32_t : 8;
                        uint32_t trb_type : 6;
                        uint32_t control : 16;
                    } __attribute__((packed)) bits;
                };

                union SlotContext {
                    uint32_t dwords[8];
                    struct {
                        uint32_t route_string : 20;
                        uint32_t speed : 4;
                        uint32_t : 1;  // reserved
                        uint32_t mtt : 1;
                        uint32_t hub : 1;
                        uint32_t context_entries : 5;

                        uint32_t max_exit_latency : 16;
                        uint32_t root_hub_port_num : 8;
                        uint32_t num_ports : 8;

                        // TT : Transaction Translator
                        uint32_t tt_hub_slot_id : 8;
                        uint32_t tt_port_num : 8;
                        uint32_t ttt : 2;
                        uint32_t : 4;  // reserved
                        uint32_t interrupter_target : 10;

                        uint32_t usb_device_address : 8;
                        uint32_t : 19;
                        uint32_t slot_state : 5;
                    } __attribute__((packed)) bits;
                } __attribute__((packed));

                union EndpointContext {
                    uint32_t dwords[8];
                    struct {
                        uint32_t ep_state : 3;
                        uint32_t : 5;
                        uint32_t mult : 2;
                        uint32_t max_primary_streams : 5;
                        uint32_t linear_stream_array : 1;
                        uint32_t interval : 8;
                        uint32_t max_esit_payload_hi : 8;

                        uint32_t : 1;
                        uint32_t error_count : 2;
                        uint32_t ep_type : 3;
                        uint32_t : 1;
                        uint32_t host_initiate_disable : 1;
                        uint32_t max_burst_size : 8;
                        uint32_t max_packet_size : 16;

                        uint32_t dequeue_cycle_state : 1;
                        uint32_t : 3;
                        uint64_t tr_dequeue_pointer : 60;

                        uint32_t average_trb_length : 16;
                        uint32_t max_esit_payload_lo : 16;
                    } __attribute__((packed)) bits;
                    TRB* TransferRingBuffer() const {
                        return reinterpret_cast<TRB*>(bits.tr_dequeue_pointer << 4);
                    }

                    void SetTransferRingBuffer(TRB* buffer) {
                        bits.tr_dequeue_pointer = reinterpret_cast<uint64_t>(buffer) >> 4;
                    }
                } __attribute__((packed));

                struct DeviceContext {
                    SlotContext slot_context;
                    EndpointContext ep_contexts[31];
                } __attribute__((packed));

                alignas(64) volatile DeviceContext* dcbaa[NUM_DEVICE_SLOT];
                DCBAAP = (uint64_t)dcbaa;

                #define RING_SIZE 8
                struct RingManager{
                    TRB buf[RING_SIZE];
                    int cycle_bit;
                    int write_index;
                };

                alignas(64) volatile RingManager cr;
                {
                    uint8_t* ptr = (uint8_t*)cr.buf;
                    int64_t i = sizeof(cr.buf);
                    while (i--) *(ptr++) = 0u;
                }
                cr.cycle_bit = 1;
                cr.write_index = 0;
                CRCR = (uint64_t)cr.buf | cr.cycle_bit;

                union EventRingSegmentTableEntry {
                    std::array<uint32_t, 4> data;
                    struct {
                        uint64_t ring_segment_base_address;  // 64 バイトアライメント

                        uint32_t ring_segment_size : 16;
                        uint32_t : 16;

                        uint32_t : 32;
                    } __attribute__((packed)) bits;
                };

#define ERSEGM_SIZE 16
                alignas(64) volatile TRB er_segment[ERSEGM_SIZE];
                alignas(64) volatile EventRingSegmentTableEntry erst[1];
                erst[0].bits.ring_segment_base_address = reinterpret_cast<uint64_t>(er_segment);
                erst[0].bits.ring_segment_size = ERSEGM_SIZE;

                ERSTSZ(0) = 1;
                ERSTBA(0) = (uint64_t)erst;

                USBCMD |= 1u;

                Log(kDebug, "Run/Stop Enabled\n");

                Log(kDebug, "USBCMD: 0x%08lx\n", USBCMD & 0xffffffffu);
                Log(kDebug, "USBSTS: 0x%08lx\n", USBSTS & 0xffffffffu);

                while (1) __asm__("hlt");

                return Error(Error::kSuccess, NULL, 0);
            }
            void Run() {}
        };
    }  // namespace xhci
}  // namespace usb