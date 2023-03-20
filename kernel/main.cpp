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
#include "pci.hpp"
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

const PixelColor kDesktopBGColor{45, 118, 237};
const PixelColor kDesktopFGColor{255, 255, 255};

// #@@range_begin(mouse_cursor)
const int kMouseCursorWidth = 15;
const int kMouseCursorHeight = 24;
const char mouse_cursor_shape[kMouseCursorHeight][kMouseCursorWidth + 1] = {
    "@              ",
    "@@             ",
    "@.@            ",
    "@..@           ",
    "@...@          ",  // 5
    "@....@         ",
    "@.....@        ",
    "@......@       ",
    "@.......@      ",
    "@........@     ",  // 10
    "@.........@    ",
    "@..........@   ",
    "@...........@  ",
    "@............@ ",
    "@......@@@@@@@@",  // 15
    "@......@       ",
    "@....@@.@      ",
    "@...@ @.@      ",
    "@..@   @.@     ",
    "@.@    @.@     ",  // 20
    "@@      @.@    ",
    "@       @.@    ",
    "         @.@   ",
    "         @@@   "  // 24
};
// #@@range_end(mouse_cursor)

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

    printk("Welcome to MikanOS! 2023/03/20 rev.002\n");

    SetLogLevel(kVerbose);

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
    Log(kDebug, "xHC mmio_base = %08lx\n", xhc_mmio_base);

    // #@@range_begin(draw_cursor)
    for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
        for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
            switch (mouse_cursor_shape[dy][dx]) {
                case '@':
                    pixel_writer->Write(600 + dx, 100 + dy, {0, 0, 0});
                    break;
                case '.':
                    pixel_writer->Write(600 + dx, 100 + dy, {255, 255, 255});
                    break;
            }
        }
    }
    // #@@range_end(draw_cursor)

    while (1) __asm__("hlt");
}
