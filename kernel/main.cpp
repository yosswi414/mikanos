/**
 * @file main.cpp
 *
 * カーネル本体のプログラムを書いたファイル．
 */

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <limits>
#include <numeric>
#include <vector>

#include "asmfunc.h"
#include "console.hpp"
#include "font.hpp"
#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "interrupt.hpp"
#include "layer.hpp"
#include "logger.hpp"
#include "memory_manager.hpp"
#include "memory_map.hpp"
#include "message.hpp"
#include "mouse.hpp"
#include "paging.hpp"
#include "pci.hpp"
#include "segment.hpp"
#include "timer.hpp"
#include "usb/xhci/xhci.hpp"
#include "window.hpp"

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

std::vector<std::shared_ptr<Window>> normal_window(0);
std::vector<unsigned int> normal_window_layer_id(0);

unsigned int counter_window_idx;
unsigned int counter_window_layer_id;

void InitializeNormalWindow() {
    // list 10.4, p.249
    normal_window.emplace_back(std::make_shared<Window>(160, 68, screen_config.pixel_format));
    DrawWindow(*normal_window.back(), "Hello World 1");
    WriteString(*normal_window.back(), {24, 28}, "Welcome to", {0, 0, 0});
    WriteString(*normal_window.back(), {24, 44}, " MikanOS world!", {0, 0, 0});

    normal_window_layer_id.push_back(layer_manager->NewLayer()
                                         .SetWindow(normal_window.back())
                                         .SetDraggable(true)
                                         .Move({300, 100})
                                         .ID());

    normal_window.emplace_back(std::make_shared<Window>(160, 68, screen_config.pixel_format));
    DrawWindow(*normal_window.back(), "Hello World 2");
    WriteString(*normal_window.back(), {24, 28}, "This is", {0, 0, 0});
    WriteString(*normal_window.back(), {24, 44}, " the second", {0, 0, 0});

    normal_window_layer_id.push_back(layer_manager->NewLayer()
                                         .SetWindow(normal_window.back())
                                         .SetDraggable(true)
                                         .Move({300, 200})
                                         .ID());
    normal_window.emplace_back(std::make_shared<Window>(160, 68, screen_config.pixel_format));
    DrawWindow(*normal_window.back(), "Hello World 3");
    WriteString(*normal_window.back(), {24, 28}, "I am", {0, 0, 0});
    WriteString(*normal_window.back(), {24, 44}, " the third win", {0, 0, 0});

    normal_window_layer_id.push_back(layer_manager->NewLayer()
                                         .SetWindow(normal_window.back())
                                         .SetDraggable(true)
                                         .Move({300, 300})
                                         .ID());

    // list 10.8, p.252
    normal_window.emplace_back(std::make_shared<Window>(160, 52, screen_config.pixel_format));
    DrawWindow(*normal_window.back(), "Count, Count");

    normal_window_layer_id.push_back(layer_manager->NewLayer()
                                         .SetWindow(normal_window.back())
                                         .SetDraggable(true)
                                         .Move({400, 200})
                                         .ID());

    // export
    counter_window_idx = normal_window.size() - 1;
    counter_window_layer_id = normal_window_layer_id.back();

    for (auto wid : normal_window_layer_id) {
        layer_manager->UpDown(wid, 1000);
    }
}

std::deque<Message> *main_queue;

void InitializeFontData(uint8_t *src, uint8_t dst[128][KERNEL_GLYPH_HEIGHT]) {
    for (int i = 0; i < 0x80; ++i) {
        for (int j = 0; j < KERNEL_GLYPH_HEIGHT; ++j) {
            dst[i][j] = *src++;
        }
    }
}

alignas(16) uint8_t font_data[128][KERNEL_GLYPH_HEIGHT];

alignas(16) uint8_t kernel_main_stack[1024 * 1024];

extern "C" void KernelMainNewStack(const FrameBufferConfig &frame_buffer_config_ref,
                                   const MemoryMap &memory_map_ref,
                                   uint8_t *font_data_ref) {
    MemoryMap memory_map{memory_map_ref};

    InitializeGraphics(frame_buffer_config_ref);
    InitializeConsole();

    InitializeFontData(font_data_ref, font_data);

    printk("Welcome to MikanOS! " __DATE__ " " __TIME__ " rev.001\n");
    SetLogLevel(kWarn);

    InitializeLAPICTimer();

    InitializeSegmentation();
    InitializePaging();
    InitializeMemoryManager(memory_map);

    ::main_queue = new std::deque<Message>(32);
    InitializeInterrupt(main_queue);

    InitializePCI();
    usb::xhci::Initialize();

    InitializeLayer();
    InitializeNormalWindow();
    InitializeMouse();

    layer_manager->Draw({{0, 0}, ScreenSize()});

    char str[128];
    unsigned int count = 0;

    // event loop
    while (true) {
        sprintf(str, "%010u", ++count);
        FillRectangle(*normal_window[counter_window_idx], {24, 28}, {KERNEL_GLYPH_WIDTH * 10, KERNEL_GLYPH_HEIGHT}, {0xc6, 0xc6, 0xc6});
        WriteString(*normal_window[counter_window_idx], {24, 28}, str, {0, 0, 0});
        layer_manager->Draw(counter_window_layer_id);

        __asm__("cli");  // Clear Interrupt Flag
        if (main_queue->size() == 0) {
            __asm__("sti");
            continue;
        }

        Message msg = main_queue->front();
        main_queue->pop_front();
        __asm__("sti");

        switch (msg.type) {
            case Message::kInterruptXHCI:
                usb::xhci::ProcessEvents();
                break;
            default:
                Log(kError, "Unknown message type: %d\n", msg.type);
        }
    }
}

extern "C" void __cxa_pure_virtual() {
    while (1) __asm__("hlt");
}