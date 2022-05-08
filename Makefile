CC:=gcc
AS:=nasm
LD:=ld
WC:=/usr/bin/wc
SUDO:=sudo

QEMU_DEBUG:=0
QEMU_OPTS:=-no-reboot -no-shutdown -d int,cpu_reset

TFTP_DIR:=/var/lib/tftpboot/

PHONY:=

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

stage1: stage1.s boot_info.inc
	$(AS) stage1.s -o _stage1
	$(Q)if [ `stat --format="%s" _stage1` -ne 512 ]; then \
		echo >&2 "Check size for stage1 failed"; \
		false;\
	fi

	mv _stage1 stage1

stage2: stage2.s
	$(AS) stage2.s -o stage2

boot_info.inc: stage2 kernel64
	$(Q)echo '%define STAGE2_LOAD_ADDR $(STAGE2_LOAD_ADDR)' > boot_info.inc
	$(Q)echo '%define STAGE2_LOAD_SEG  $(STAGE2_LOAD_SEG)' >> boot_info.inc

	$(Q)echo -n '%define LOADER_SEC_SIZE ' >> boot_info.inc
	$(Q)expr \( `stat --format="%s" stage2` + 511 \) / 512 >> boot_info.inc

	$(Q)echo -n '%define KERNEL64_SEC_SIZE ' >> boot_info.inc
	$(Q)expr \( `stat --format="%s" kernel64` + 511 \) / 512 >> boot_info.inc

libkernel64.a: kernel/Cargo.toml kernel/Cargo.lock kernel/src/* FORCE
	$(Q)cd kernel && cargo rustc --lib --release -vv -- -C soft-float -C lto --emit link=../$@

kernel64: kernel/x86_64.ld libkernel64.a
	$(LD) --gc-sections -T kernel/x86_64.ld -o $@ libkernel64.a

floppy.bin: stage1 stage2 kernel64
	cat stage1 stage2 kernel64 > floppy.bin
	$(Q)$(MAKE) -s stats

PHONY += stats
stats:
	$(Q)echo -e "\n\033[92mKernel ELF stats:\033[0m"
	$(Q)size kernel64 --format=SysV

PHONY += clean
clean:
	$(Q)rm -f stage1 stage1.o stage2 stage2.o boot_info.inc kernel64 libkernel64.a floppy.bin
	$(Q)cd kernel && cargo clean

PHONY += run
run: floppy.bin
	qemu-system-x86_64 -hda floppy.bin $(QEMU_OPTS)

PHONY += install_tftp
install_tftp: floppy.bin
	$(Q)$(SUDO) cp $< "$(TFTP_DIR)"
	$(Q)echo "(CP) $< $(TFTP_DIR)"

FORCE:

.PHONY:= $(PHONY)
