AS:=nasm
WC:=/usr/bin/wc

QEMU_DEBUG:=0
QEMU_OPTS:=

PHONY:=

ifeq ($(QEMU_DEBUG),1)
	QEMU_OPTS+=-S -s 
endif

all: stage1 stage2 floppy.bin

stage1: stage1.s
	$(AS) stage1.s -o _stage1
	if [ `stat --format="%s" _stage1` -ne 512 ]; then \
		echo >&2 "Check size for stage1 failed"; \
		false;\
	fi

	mv _stage1 stage1

stage2: stage2.s
	$(AS) stage2.s -o stage2

floppy.bin: stage1 stage2
	cat stage1 stage2 > floppy.bin

PHONY += clean
clean:
	@rm -f stage1

PHONY += run
run: floppy.bin
	qemu-system-x86_64 -hda floppy.bin $(QEMU_OPTS)

.PHONY:= $(PHONY)
