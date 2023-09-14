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
#include "mouse.hpp"
#include "paging.hpp"
#include "pci.hpp"
#include "queue.hpp"
#include "segment.hpp"
#include "timer.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/device.hpp"
#include "usb/memory.hpp"
#include "usb/xhci/trb.hpp"
#include "usb/xhci/xhci.hpp"
#include "window.hpp"

// void* operator new(size_t size, void *buf) noexcept {
//     return buf;
// }

// void operator delete(void *obj) noexcept {
// }

char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter *pixel_writer;

// #@@range_begin(console_buf)
char console_buf[sizeof(Console)];
Console *console;
// #@@range_end(console_buf)

int printk(const char *format, ...) {
    va_list ap;
    int result;
    char s[1024];

    va_start(ap, format);
    result = vsprintf(s, format, ap);
    va_end(ap);

    StartLAPICTimer();
    console->PutString(s);
    auto elapsed = LAPICTimerElapsed();
    StopLAPICTimer();

    sprintf(s, "[%9d] ", elapsed);
    console->PutString(s);
    return result;
}

char memory_manager_buf[sizeof(BitmapMemoryManager)];
BitmapMemoryManager *memory_manager;

// [list 6.25, p.156]
// #@@range_begin(mouse_observer)
// char mouse_cursor_buf[sizeof(MouseCursor)];
// MouseCursor *mouse_cursor;
unsigned int mouse_layer_id;

Vector2D<int> screen_size;
Vector2D<int> mouse_position;

void MouseObserver(int8_t displacement_x, int8_t displacement_y) {
    auto newpos = mouse_position + Vector2D<int>{displacement_x, displacement_y};
    newpos = ElementMin(newpos, screen_size + Vector2D<int>{-1, -1});
    newpos = ElementMax(newpos, {0, 0});
    mouse_position = newpos;
    // mouse_cursor->MoveRelative({displacement_x, displacement_y});
    layer_manager->Move(mouse_layer_id, mouse_position);

    // measures time to draw mouse icon layer
    StartLAPICTimer();

    layer_manager->Draw();

    auto elapsed = LAPICTimerElapsed();
    StopLAPICTimer();
    static int cMouse = 0;
    (++cMouse) %= 5;
    if (!cMouse) {
        printk("MouseObserver: elapsed = %u\t(%d,%d) +(%d,%d)\n", elapsed, mouse_position.x, mouse_position.y, displacement_x, displacement_y);
    }
}
// #@@range_end(mouse_observer)

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

// #@@range_begin(xhci_handler)
usb::xhci::Controller *xhc;

struct Message {
    enum Type {
        kInterruptXHCI,
    } type;
};

ArrayQueue<Message> *main_queue;

__attribute__((interrupt)) void IntHandlerXHCI(InterruptFrame *frame) {
    // while (xhc->PrimaryEventRing()->HasFront()) {
    //     if (auto err = ProcessEvent(*xhc)) Log(kError, "Error while ProcessEvent: %s at %s:%d\n", err.Name(), err.File(), err.Line());
    // }
    main_queue->Push(Message{Message::kInterruptXHCI});
    interrupt::Controller().NotifyEndOfInterrupt();
}
// #@@range_end(xhci_handler)

alignas(16) uint8_t font_data[128][KERNEL_GLYPH_HEIGHT];
// uint64_t font_data_size;

alignas(16) uint8_t kernel_main_stack[1024 * 1024];

extern "C" void KernelMainNewStack(const FrameBufferConfig &frame_buffer_config_ref,
                                   const MemoryMap &memory_map_ref,
                                   uint8_t *font_data_ref) {
    FrameBufferConfig frame_buffer_config{frame_buffer_config_ref};
    MemoryMap memory_map{memory_map_ref};

    // font_data_start = font_data;
    // font_data_size = (uint64_t)KERNEL_GLYPH_HEIGHT * 128; // 128: ascii 0x00 - 0x7f

    for (int i = 0; i < 0x80; ++i) {
        for (int j = 0; j < KERNEL_GLYPH_HEIGHT; ++j) {
            font_data[i][j] = *font_data_ref++;
        }
    }

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

    DrawDesktop(*pixel_writer);

    console = new (console_buf) Console{kDesktopFGColor, kDesktopBGColor};
    console->SetWriter(pixel_writer);

    printk("Welcome to MikanOS! " __DATE__ " " __TIME__ " rev.001\n");

    SetLogLevel(kWarn);

    InitializeLAPICTimer();

    // Log(kInfo, "GetCS: 0x%08lx\n", GetCS());

    SetupSegments();

    // kernel_cs: also referred in call of SetIDTEntry() in this routine
    const uint16_t kernel_cs = 1 << 3;  // points to gdt[1]: code segment
    const uint16_t kernel_ss = 2 << 3;  // points to gdt[2]: data segment
    SetDSAll(0);                        // point to null descriptor gdt[0]; DS and ES won't be used in x86-64 64-bit mode
                                        // and FS, GS too unless explicitly used by programmer (p.193)
    SetCSSS(kernel_cs, kernel_ss);

    // Log(kInfo, "GetCS: 0x%08lx\n", GetCS());

    SetupIdentityPageTable();

    // #@@range_begin(mark_allocated)
    ::memory_manager = new (memory_manager_buf) BitmapMemoryManager;

    // #@@range_begin(print_memory_map)
    // printk("memory_map: %p\n", &memory_map);
    // assuming identity mapping
    const auto memory_map_base = reinterpret_cast<uintptr_t>(memory_map.buffer);
    uintptr_t available_end = 0;
    for (uintptr_t itr = memory_map_base;
         itr < memory_map_base + memory_map.map_size;
         itr += memory_map.descriptor_size) {
        auto desc = reinterpret_cast<const MemoryDescriptor *>(itr);

        if (available_end < desc->physical_start) {
            memory_manager->MarkAllocated(
                FrameID{available_end / kBytesPerFrame},
                (desc->physical_start - available_end) / kBytesPerFrame);
        }

        const auto physical_end = desc->physical_start + desc->number_of_pages * kUEFIPageSize;
        if (IsAvailable(static_cast<MemoryType>(desc->type))) {
            available_end = physical_end;
            Log(kInfo, "type = %u, phys = %08lx - %08lx, pages = %lu,\tattr = %016llx\n",
                desc->type,
                desc->physical_start,
                desc->physical_start + desc->number_of_pages * kUEFIPageSize - 1,
                desc->number_of_pages,
                desc->attribute);
        } else {
            memory_manager->MarkAllocated(
                FrameID{desc->physical_start / kBytesPerFrame},
                desc->number_of_pages * kUEFIPageSize / kBytesPerFrame);
        }
    }
    memory_manager->SetMemoryRange(FrameID{1}, FrameID{available_end / kBytesPerFrame});

    if (auto err = InitializeHeap(*memory_manager)) {
        Log(kError, "failed to allocate pages: %s at %s:%d\n", err.Name(), err.File(), err.Line());
        exit(1);
    }

    Log(kDebug, "memory frame ids: 1 to %d (0x%lx)\n", available_end / kBytesPerFrame, available_end / kBytesPerFrame);
    Log(kInfo, "memory capacity: %d MiB\n", available_end / (1_MiB));
    // #@@range_end(mark_allocated)

    // const std::array available_memory_types{
    //     MemoryType::kEfiBootServicesCode,
    //     MemoryType::kEfiBootServicesData,
    //     MemoryType::kEfiConventionalMemory,
    // };

    // #@@range_begin(new_mouse_cursor)
    // mouse_cursor = new (mouse_cursor_buf) MouseCursor{pixel_writer, kDesktopBGColor, {600, 500}};
    // #@@range_end(new_mouse_cursor)

    std::array<Message, 32> main_queue_data;
    ArrayQueue<Message> main_queue{main_queue_data};
    ::main_queue = &main_queue;

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

    SetIDTEntry(idt[InterruptVector::kXHCI], MakeIDTAttr(DescriptorType::kInterruptGate, 0),
                reinterpret_cast<uint64_t>(IntHandlerXHCI), kernel_cs);
    LoadIDT(sizeof(idt) - 1, reinterpret_cast<uintptr_t>(&idt[0]));

    // [list 7.8, p.170]
    // bsp: BootStrap Processor
    const uint8_t bsp_local_apic_id = interrupt::Controller().GetLAPICID();
    pci::ConfigureMSIFixedDestination(
        *xhc_dev, bsp_local_apic_id,
        pci::MSITriggerMode::kLevel,
        pci::MSIDeliveryMode::kFixed,
        InterruptVector::kXHCI, 0);

    // PCI デバイス (*xhc_dev) の BAR0 レジスタを読み取る
    const WithError<uint64_t> xhc_bar = pci::ReadBar(*xhc_dev, 0);
    Log(kDebug, "ReadBar: %s\n", xhc_bar.error.Name());
    // 下位 4 ビットに BAR のフラグがあるので ~0xf でマスクして除去
    const uint64_t xhc_mmio_base = xhc_bar.value & ~static_cast<uint64_t>(0xf);
    Log(kDebug, "xhc_bar = 0x%016lx\n", xhc_bar.value);
    Log(kDebug, "xHC mmio_base = 0x%08lx\n", xhc_mmio_base);

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

    ::xhc = &xhc;

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

    const int kFrameWidth = frame_buffer_config.horizontal_resolution;
    const int kFrameHeight = frame_buffer_config.vertical_resolution;

    auto bgwindow = std::make_shared<Window>(kFrameWidth, kFrameHeight, frame_buffer_config.pixel_format);
    // auto bgwriter = bgwindow->Writer();

    // DrawDesktop(*bgwriter);
    DrawDesktop(*bgwindow);
    console->SetWindow(bgwindow);

    auto mouse_window = std::make_shared<Window>(kMouseCursorWidth, kMouseCursorHeight, frame_buffer_config.pixel_format);
    mouse_window->SetTransparentColor(kMouseTransparentColor);
    // DrawMouseCursor(mouse_window->Writer(), {0, 0});
    DrawMouseCursor(mouse_window.get(), {0, 0});

    screen_size = {
        static_cast<int>(frame_buffer_config.horizontal_resolution),
        static_cast<int>(frame_buffer_config.vertical_resolution)
    };

    FrameBuffer screen;
    if (auto err = screen.Initialize(frame_buffer_config)) {
        Log(kError, "failed to initialize frame buffer: %s at %s:%d\n",
            err.Name(), err.File(), err.Line());
    }

    layer_manager = new LayerManager;

    // layer_manager->SetWriter(pixel_writer);
    layer_manager->SetWriter(&screen);

    auto bglayer_id = layer_manager->NewLayer()
                          .SetWindow(bgwindow)
                          .Move({0, 0})
                          .ID();
    mouse_layer_id = layer_manager->NewLayer()
                         .SetWindow(mouse_window)
                         .Move({200, 200})
                         .ID();

    // list 10.4, p.249
    auto main_window = std::make_shared<Window>(160, 68, frame_buffer_config.pixel_format);
    DrawWindow(*main_window, "Hello World");
    WriteString(*main_window, {24, 28}, "Welcome to", {0, 0, 0});
    WriteString(*main_window, {24, 44}, " MikanOS world!", {0, 0, 0});

    auto main_window_layer_id = layer_manager->NewLayer()
                                    .SetWindow(main_window)
                                    .Move({300, 100})
                                    .ID();

    // list 10.8, p.252
    auto counter_window = std::make_shared<Window>(160, 52, frame_buffer_config.pixel_format);
    DrawWindow(*counter_window, "Count, Count");

    char str[128];
    unsigned int count = 0;

    auto counter_window_layer_id = layer_manager->NewLayer()
                                    .SetWindow(counter_window)
                                    .Move({400, 200})
                                    .ID();

    layer_manager->UpDown(bglayer_id, 0);
    layer_manager->UpDown(mouse_layer_id, 1);
    layer_manager->UpDown(main_window_layer_id, 1);
    layer_manager->UpDown(counter_window_layer_id, 1);
    layer_manager->Draw();

    // event loop
    while (true) {
        sprintf(str, "%010u", ++count);
        FillRectangle(*counter_window, {24, 28}, {KERNEL_GLYPH_WIDTH * 10, KERNEL_GLYPH_HEIGHT}, {0xc6, 0xc6, 0xc6});
        WriteString(*counter_window, {24, 28}, str, {0, 0, 0});
        layer_manager->Draw();
        
        __asm__("cli");  // Clear Interrupt Flag
        if (main_queue.Count() == 0) {
            /* sti 命令と直後の 1 命令の間では割り込みが起きないという仕様を活用するため
             * sti と hlt を並べることが肝要
             */
            // __asm__("sti\n\thlt");
            __asm__("sti");
            continue;
        }

        Message msg = main_queue.Front();
        main_queue.Pop();
        __asm__("sti");

        switch (msg.type) {
            case Message::kInterruptXHCI:
                while (xhc.PrimaryEventRing()->HasFront()) {
                    if (auto err = ProcessEvent(xhc)) {
                        Log(kError, "Error while ProcessEvent: %s at %s:%d\n", err.Name(), err.File(), err.Line());
                    }
                }
                break;
            default:
                Log(kError, "Unknown message type: %d\n", msg.type);
        }
    }
}

extern "C" void __cxa_pure_virtual() {
    while (1) __asm__("hlt");
}