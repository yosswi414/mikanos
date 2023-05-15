TARGET = kernel.elf
# OBJS = main.o graphics.o font.o hankaku.o newlib_support.o console.o pci.o asmfunc.o logger.o
OBJS = main.o graphics.o font.o newlib_support.o console.o pci.o asmfunc.o logger.o mouse.o
DEPENDS = $(join $(dir $(OBJS)),$(addprefix .,$(notdir $(OBJS:.o=.d))))

CFLAGS   += -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone
CXXFLAGS += -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone \
            -fno-exceptions -fno-rtti -std=c++17
LDFLAGS  += --entry KernelMain -z norelro --image-base 0x100000 --static

BASE = /home/yosswi414/mikanOS
BASEDIR = $(BASE)/osbook/devenv/x86_64-elf
EDK2DIR = $(BASE)/edk2
KRNLDIR = $(BASE)/workspace/mikanos/kernel
CPPFLAGS = \
    -I$(BASEDIR)/include/c++/v1 -I$(BASEDIR)/include -I$(BASEDIR)/include/freetype2 \
    -I$(EDK2DIR)/MdePkg/Include -I$(EDK2DIR)/MdePkg/Include/X64 \
	-I$(KRNLDIR) \
    -nostdlibinc -D__ELF__ -D_LDBL_EQ_DBL -D_GNU_SOURCE -D_POSIX_TIMERS \
    -DEFIAPI='__attribute__((ms_abi))'
LDFLAGS += -L$(BASEDIR)/lib

# OLD_SHELL := $(SHELL)
# SHELL = $(warning [Making: $@]   [Dependencies: $^]   [Changed: $?])$(OLD_SHELL)

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	rm -rf *.o
	rm -rf *.d

kernel.elf: $(OBJS) Makefile
	ld.lld $(LDFLAGS) -o kernel.elf $(OBJS) -lc

%.o: %.cpp Makefile
	clang++ $(CPPFLAGS) $(CXXFLAGS) -c $<

.%.d: %.cpp
	clang++ $(CPPFLAGS) $(CXXFLAGS) -MM $< > $@
	$(eval OBJ = $(<:.cpp=.o))
	sed --in-place 's|$(notdir $(OBJ))|$(OBJ)|' $@

%.o: %.c Makefile
	clang $(CPPFLAGS) $(CFLAGS) -c $<

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
