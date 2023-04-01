CC:=gcc
AR:=ar
AS:=as
LD:=ld
OBJCOPY:=objcopy
STRIP:=strip
SUDO:=sudo

CFLAGS:=-Wall -Wextra -nostdlib -ffreestanding -mno-red-zone -fPIC -std=c17 \
	-fno-stack-protector -O2 -mno-sse -mno-avx -mno-mmx -mgeneral-regs-only \
	-g3 -ggdb

LDFLAGS:=-nostdlib

ASFLAGS:=--64

QEMU_DEBUG:=0
QEMU_OPTS:=-no-reboot -no-shutdown -d int,cpu_reset,pcall

TFTP_DIR:=/var/lib/tftpboot/

PHONY:=

KERNEL_SRC:=kernel.c textmode.c memory.c abort.c irq.c std.c
KERNEL_SRC_ASM:=irq_handler.s
KERNEL_OBJS:=$(patsubst %.c,%.o,$(KERNEL_SRC))
KERNEL_OBJS+=$(patsubst %.s,%.o,$(KERNEL_SRC_ASM))

# DO NOT CHANGE THIS!
# For now this cannot be changed because stage2.s depends on this address.
# This might change in the future if we decide to generate stage2.s with
# this constant instead of hardcoding - for now its easier that way.
STAGE2_LOAD_ADDR:=0x7e00
STAGE2_LOAD_SEG:=0x0000

ifeq ($(QEMU_DEBUG),1)
	QEMU_OPTS+= -S -s 
endif

ifeq ("$(V)", "1")
	Q:=
else
	Q:=@
endif

all: stage1 stage2 kernel64 floppy.bin

depend: .depend

.depend: $(KERNEL_SRC)
	$(Q)rm -f .depend
	$(Q)$(CC) $(CFLAGS) -MM $^ > .depend

include .depend

stage1: stage1.s boot_info.inc
	@echo -e "  **\t Building stage 1 bootloader"
	@echo -e "NASM\t" stage1,s
	$(Q)nasm stage1.s -o _stage1
	$(Q)if [ `stat --format="%s" _stage1` -ne 512 ]; then \
		echo >&2 "Check size for stage1 failed"; \
		false;\
	fi

	$(Q)mv _stage1 stage1

stage2: stage2.s
	@echo -e "  **\t Building stage 2 bootloader"
	@echo -e "NASM\t" $<
	$(Q)nasm stage2.s -o stage2

boot_info.inc: stage2 kernel64
	$(Q)echo '%define STAGE2_LOAD_ADDR $(STAGE2_LOAD_ADDR)' > boot_info.inc
	$(Q)echo '%define STAGE2_LOAD_SEG  $(STAGE2_LOAD_SEG)' >> boot_info.inc

	$(Q)echo -n '%define LOADER_SEC_SIZE ' >> boot_info.inc
	$(Q)expr \( `stat --format="%s" stage2` + 511 \) / 512 >> boot_info.inc

	$(Q)echo -n '%define KERNEL64_SEC_SIZE ' >> boot_info.inc
	$(Q)expr \( `stat --format="%s" kernel64` + 511 \) / 512 >> boot_info.inc

%.o: %.c
	@echo -e "  CC\t" $<
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	@echo -e "  AS\t" $<
	$(Q)$(AS) $(ASFLAGS) -c $< -o $@

kernel64: x86_64.ld $(KERNEL_OBJS) types.h
	$(Q)$(LD) $(LDFLAGS) --gc-sections -T x86_64.ld -o $@ $(KERNEL_OBJS)
	$(Q)$(OBJCOPY) --only-keep-debug $@ $@.syms
	$(Q)$(STRIP) --strip-all $@


floppy.bin: stage1 stage2 kernel64
	$(Q)cat stage1 stage2 kernel64 > floppy.bin
	$(Q)$(MAKE) -s stats

PHONY += stats
stats:
	$(Q)echo -e "\n\033[92mKernel ELF stats:\033[0m"
	$(Q)size kernel64 --format=SysV

PHONY += clean
clean:
	$(Q)rm -f stage1 stage1.o stage2 stage2.o boot_info.inc kernel64 kernel64.syms libkernel64.a floppy.bin
	$(Q)rm -f .depend
	$(Q)rm -f $(KERNEL_OBJS)

PHONY += run
run: floppy.bin
	$(Q) qemu-system-x86_64 -hda floppy.bin $(QEMU_OPTS)

PHONY += install_tftp
install_tftp: floppy.bin
	$(Q)$(SUDO) cp $< "$(TFTP_DIR)"
	$(Q)echo "(CP) $< $(TFTP_DIR)"

FORCE:

.PHONY:= $(PHONY)
