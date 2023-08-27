TARGET = kernel.elf
# OBJS = main.o graphics.o font.o newlib_support.o console.o pci.o asmfunc.o logger.o mouse.o libcxx_support.o \
# 	usb/memory.o usb/device.o usb/xhci/ring.o usb/xhci/trb.o usb/xhci/xhci.o \
# 	usb/xhci/port.o usb/xhci/device.o usb/xhci/devmgr.o usb/xhci/registers.o \
# 	usb/classdriver/base.o usb/classdriver/hid.o usb/classdriver/keyboard.o \
# 	usb/classdriver/mouse.o \
# 	interrupt.o segment.o paging.o memory_manager.o
OBJS = $(subst .c,.o,$(shell find . -type f -name "*.c")) $(subst .cpp,.o,$(shell find . -type f -name "*.cpp")) asmfunc.o

DEPENDS = $(join $(dir $(OBJS)),$(addprefix .,$(notdir $(OBJS:.o=.d))))

BASE = $(HOME)/mikanOS
BASE_DIR = $(BASE)/osbook/devenv/x86_64-elf
EDK2_DIR = $(BASE)/edk2
KERNEL_DIR = $(BASE)/workspace/mikanos/kernel

CPPFLAGS += -I. -I$(BASE_DIR)/include/c++/v1 -I$(BASE_DIR)/include -nostdlibinc -D__ELF__ -D_LDBL_EQ_DBL -D_GNU_SOURCE -D_POSIX_TIMERS \
     -DEFIAPI='__attribute__((ms_abi))'
CFLAGS   += -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone
CXXFLAGS += -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone \
            -fno-exceptions -fno-rtti -std=c++17
LDFLAGS  += --entry KernelMain -z norelro --image-base 0x100000 --static -L$(BASE_DIR)/lib


# CPPFLAGS += -I. \
    # -I$(BASE_DIR)/include/c++/v1 -I$(BASE_DIR)/include -I$(BASE_DIR)/include/freetype2 \
    # -I$(EDK2_DIR)/MdePkg/Include -I$(EDK2_DIR)/MdePkg/Include/X64 \
	# -I$(KERNEL_DIR) \

# OLD_SHELL := $(SHELL)
# SHELL = $(warning [Making: $@]   [Dependencies: $^]   [Changed: $?])$(OLD_SHELL)

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	find $(KERNEL_DIR) \( -name "*.o" \) -delete
	rm kernel.elf kernel.lst

kernel.elf: $(OBJS) Makefile
	ld.lld $(LDFLAGS) -o kernel.elf $(OBJS) -lc -lc++ -lc++abi

%.o: %.cpp Makefile
	clang++ $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

.%.d: %.cpp
	clang++ $(CPPFLAGS) $(CXXFLAGS) -MM $< > $@
	$(eval OBJ = $(<:.cpp=.o))
	sed --in-place 's|$(notdir $(OBJ))|$(OBJ)|' $@

%.o: %.c Makefile
	clang $(CPPFLAGS) $(CFLAGS) -c $< -o $@

.%.d: %.c
	clang $(CPPFLAGS) $(CFLAGS) -MM $< > $@
	$(eval OBJ = $(<:.c=.o))
	sed --in-place 's|$(notdir $(OBJ))|$(OBJ)|' $@

%.o: %.asm Makefile
	nasm -f elf64 -o $@ $<

hankaku.bin: hankaku.txt
	../tools/makefont.py -o $@ $<

hankaku.o: hankaku.bin
	objcopy -I binary -O elf64-x86-64 -B i386:x86-64 $< $@

.%.d: %.bin
	touch $@

.PHONY: depends
depends:
	$(MAKE) $(DEPENDS)

-include $(DEPENDS)
