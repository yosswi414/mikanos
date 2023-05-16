/**
 * @file main.cpp
 *
 * カーネル本体のプログラムを書いたファイル．
 */

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <numeric>
#include <vector>

// #@@range_begin(includes)
#include "console.hpp"
#include "font.hpp"
#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "logger.hpp"
#include "mouse.hpp"
#include "pci.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/device.hpp"
#include "usb/memory.hpp"
#include "usb/xhci/trb.hpp"
#include "usb/xhci/xhci.hpp"
// #@@range_end(includes)

// void* operator new(size_t size, void *buf) noexcept {
//     return buf;
// }

void operator delete(void *obj) noexcept {
}

char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter *pixel_writer;

// #@@range_begin(console_buf)
char console_buf[sizeof(Console)];
Console *console;
// #@@range_end(console_buf)

// #@@range_begin(printk)
int printk(const char *format, ...) {
    va_list ap;
    int result;
    char s[1024];

    va_start(ap, format);
    result = vsprintf(s, format, ap);
    va_end(ap);

    console->PutString(s);
    return result;
}
// #@@range_end(printk)

// [list 6.25, p.156]
// #@@range_begin(mouse_observer)
char mouse_cursor_buf[sizeof(MouseCursor)];
MouseCursor *mouse_cursor;

void MouseObserver(int8_t displacement_x, int8_t displacement_y) {
    mouse_cursor->MoveRelative({displacement_x, displacement_y});
}
// #@@range_end(mouse_observer)

const PixelColor kDesktopBGColor{45, 118, 237};
const PixelColor kDesktopFGColor{255, 255, 255};

// #@@range_begin(switch_echi2xhci)
// Intel Panther Point でデフォルトの EHCI 制御から xHCI 制御に切り替える特殊処理
void SwitchEhci2Xhci(const pci::Device &xhc_dev) {
    bool intel_ehc_exist = false;
    for (int i = 0; i < pci::num_device; ++i) {
        if (pci::devices[i].class_code.Match(0x0cu, 0x03u, 0x20u) /* EHCI */ &&
            0x8086 == pci::ReadVendorId(pci::devices[i])) {
            intel_ehc_exist = true;
            break;
        }
    }
    if (!intel_ehc_exist) {
        return;
    }
    // レジスタ (参考文献[9] 17.1.33 - 17.1.36) (p.154)
    uint32_t superspeed_ports = pci::ReadConfReg(xhc_dev, 0xdc);  // USB3PRM (SuperSpeed が有効なポートが set されている)
    pci::WriteConfReg(xhc_dev, 0xd8, superspeed_ports);           // USB3_PSSEN (USB 3.0 Port SuperSpeed Enable: set したポートに対し SuperSpeed が有効になる)
    uint32_t ehci2xhci_ports = pci::ReadConfReg(xhc_dev, 0xd4);   // XUSB2PRM (xHCI モードが有効なポートが set されている)
    pci::WriteConfReg(xhc_dev, 0xd0, ehci2xhci_ports);            // XUSB2PR (xHC USB 2.0 Port Routing: 各ビットが USB ポート一つに対応し、set したポートは xHCI モードになる)
    Log(kDebug, "SwitchEhci2Xhci: SS = %02, xHCI = %02x\n",
        superspeed_ports, ehci2xhci_ports);
}
// #@@range_end(switch_echi2xhci)

uint8_t *font_data_start;
uint64_t font_data_size;

extern "C" void KernelMain(const FrameBufferConfig &frame_buffer_config, uint8_t &font_data) {
    switch (frame_buffer_config.pixel_format) {
        case kPixelRGBResv8BitPerColor:
            pixel_writer = new (pixel_writer_buf)
                RGBResv8BitPerColorPixelWriter{frame_buffer_config};
            break;
        case kPixelBGRResv8BitPerColor:
            pixel_writer = new (pixel_writer_buf)
                BGRResv8BitPerColorPixelWriter{frame_buffer_config};
            break;
    }

    const int kFrameHeight = frame_buffer_config.vertical_resolution;
    const int kFrameWidth = frame_buffer_config.horizontal_resolution;

    font_data_start = &font_data;
    font_data_size = (uint64_t)KERNEL_GLYPH_HEIGHT * 128;

    // #@@range_begin(draw_desktop)
    FillRectangle(*pixel_writer, {0, 0}, {kFrameWidth, kFrameHeight - 50}, kDesktopBGColor);
    FillRectangle(*pixel_writer, {0, kFrameHeight - 50}, {kFrameWidth, 50}, {1, 8, 17});
    FillRectangle(*pixel_writer, {0, kFrameHeight - 50}, {kFrameWidth / 5, 50}, {80, 80, 80});
    DrawRectangle(*pixel_writer, {10, kFrameHeight - 40}, {30, 30}, {160, 160, 160});
    // #@@range_end(draw_desktop)

    // #@@range_begin(new_console)
    console = new (console_buf) Console{*pixel_writer, kDesktopFGColor, kDesktopBGColor};
    // #@@range_end(new_console)

    printk("Welcome to MikanOS! 2023/05/02 rev.002\n");

    SetLogLevel(kInfo);

    // #@@range_begin(new_mouse_cursor)
    mouse_cursor = new (mouse_cursor_buf) MouseCursor{pixel_writer, kDesktopBGColor, {300, 200}};
    // #@@range_end(new_mouse_cursor)

    auto err = pci::ScanAllBus();
    Log(kDebug, "ScanAllBus: %s\n", err.Name());

    for (int i = 0; i < pci::num_device; ++i) {
        const auto &dev = pci::devices[i];
        auto vendor_id = pci::ReadVendorId(dev.bus, dev.device, dev.function);
        auto class_code = pci::ReadClassCode(dev.bus, dev.device, dev.function);
        Log(kDebug, "%d.%d.%d: vend %04x, class %08x, head %02x", dev.bus, dev.device, dev.function, vendor_id, class_code, dev.header_type);
        if (dev.class_code.Match(0x0cu, 0x03u, 0x30u)) {  // 0x0c (Serial Bus Controller) / 0x03 (USB Controller) / 0x30 (xHCI)
            Log(kDebug, " Intel");
        }
        Log(kDebug, "\n");
    }

    pci::Device *xhc_dev = nullptr;
    for (int i = 0; i < pci::num_device; ++i) {
        // all xHCI controllers will have 0x0c, 0x03, 0x30
        if (pci::devices[i].class_code.Match(0x0cu, 0x03u, 0x30u)) {
            xhc_dev = &pci::devices[i];
            if (0x8086 == pci::ReadVendorId(*xhc_dev)) break;
        }
    }
    if (xhc_dev) {
        Log(kInfo, "xHC has been found: %d.%d.%d (vend: %04x)\n", xhc_dev->bus, xhc_dev->device, xhc_dev->function, pci::ReadVendorId(*xhc_dev));
    }

    // PCI デバイス (*xhc_dev) の BAR0 レジスタを読み取る
    const WithError<uint64_t> xhc_bar = pci::ReadBar(*xhc_dev, 0);
    Log(kDebug, "ReadBar: %s\n", xhc_bar.error.Name());
    // 下位 4 ビットに BAR のフラグがあるので ~0xf でマスクして除去
    const uint64_t xhc_mmio_base = xhc_bar.value & ~static_cast<uint64_t>(0xf);
    Log(kDebug, "xhc_bar = 0x%016lx\n", xhc_bar.value);
    Log(kDebug, "xHC mmio_base = 0x%08lx\n", xhc_mmio_base);

    // #@@range_begin(init_xhc)
    // xHCI 規格にしたがったホストコントローラを制御するためのクラス (p.153)
    usb::xhci::Controller xhc{xhc_mmio_base};
    if (0x8086 == pci::ReadVendorId(*xhc_dev)) SwitchEhci2Xhci(*xhc_dev);
    {
        // xHC がリセットされた後、動作に必要な設定が行われる (p.153)
        auto err = xhc.Initialize();
        Log(kDebug, "xhc.Initialize: %s\n", err.Name());
    }
    Log(kInfo, "xHC starting\n");
    xhc.Run();
    // #@@range_end(init_xhc)

    // unsigned long long cnt = 0, ul = 1.6e7;
    // double mul = 1.7;
    // int dx = 1, dy = 1;
    // const int &x = mouse_cursor->getPos().x;
    // const int &y = mouse_cursor->getPos().y;

    // while(1){
    //     if (++cnt > ul) {
    //         cnt = 0;
    //         if (x < 0) dx = 1, ul = (double)ul / mul;
    //         if (x > kFrameWidth) dx = -1, ul = (double)ul / mul;
    //         if (y < 0) dy = 1, ul = (double)ul / mul;
    //         if (y > kFrameHeight) dy = -1, ul = (double)ul / mul;
    //         mouse_cursor->MoveRelative({dx, dy});
    //     }
    // }

    // [list 6.23, p.155]
    // #@@range_begin(configure_port)
    usb::HIDMouseDriver::default_observer = MouseObserver;

    for (int i = 1; i <= xhc.MaxPorts(); ++i) {
        auto port = xhc.PortAt(i);
        Log(kDebug, "Port %d: IsConnected=%d\n", i, port.IsConnected());
        if (port.IsConnected()) {
            if (auto err = ConfigurePort(xhc, port)) {
                Log(kError, "failed to configure port: %s at %s:%d\n", err.Name(), err.File(), err.Line());
                continue;
            }
        }
    }
    // #@@range_end(configure_port)

    // [list 6.24, p.155]
    // #@@range_begin(receive_event)
    while (1) {
        if (auto err = ProcessEvent(xhc)) {
            Log(kError, "Error while ProcessEvent: %s at %s:%d\n", err.Name(), err.File(), err.Line());
        }
    }
    // #@@range_end(receive_event)

    while (1) __asm__("hlt");
}

extern "C" void __cxa_pure_virtual() {
    while (1) __asm__("hlt");
}