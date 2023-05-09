#pragma once

#include "error.hpp"
#include "logger.hpp"

namespace usb{
    namespace xhci{
        struct Controller{
            uint64_t base;
            const size_t kDeviceSize = 8;
            Controller(const uint64_t& addr) : base(addr) {}

#define CAPLEN *(uint64_t*)(base + 0x0)
#define USBCMD *(uint64_t*)(base + 0x20)
#define USBSTS *(uint64_t*)(base + 0x24)

            Error Initialize() {
                if (base == 0) return Error(Error::kEmpty, NULL, 0);
                Log(kDebug, "xhci.hpp rev:001\n");
                Log(kDebug, "CAPLEN 1: 0x%08lx\n", CAPLEN & 0xffffffffull);
                Log(kDebug, "USBSTS 1: 0x%08lx\n", USBSTS & 0xffffffffull);
                Log(kDebug, "USBCMD 1: 0x%08lx\n", USBCMD & 0xffffffffull);

                bool HCError = USBSTS & (1u << 12);
                bool CNReady = USBSTS & (1u << 11);

                bool HCHalted = USBSTS & 1u;
                Log(kDebug, "HCHalted: %s\n", HCHalted ? "True" : "False");
                Log(kDebug, "HCError:  %s\n", HCError ? "True" : "False");
                Log(kDebug, "CNReady:  %s\n", CNReady ? "True" : "False");

                uint64_t tmp = USBCMD | (1u << 1);
                USBCMD = tmp;
                volatile uint64_t cnt = 1e8;
                while (cnt > 0 && USBCMD & (1ull << 1)) { --cnt; }
                if (cnt == 0)
                    Log(kDebug, "USBCMD timeout\n");
                else
                    cnt = 1e8;
                while (cnt > 0 && USBSTS & (1ull << 11)) { --cnt; }
                if (cnt == 0)
                    Log(kDebug, "USBSTS timeout\n");
                else
                    Log(kDebug, "Controller Ready!\n");

                Log(kDebug, "USBSTS 2: 0x%08lx\n", USBSTS);
                Log(kDebug, "USBCMD 2: 0x%08lx\n", USBCMD);

                return Error(Error::kSuccess, NULL, 0);
            }
            void Run(){}
        };
    }  // namespace xhci
}