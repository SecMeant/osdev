AS:=nasm
WC:=/usr/bin/wc

PHONY:=

all: stage1

stage1: stage1.s
	$(AS) stage1.s -o _stage1
	if [ `stat --format="%s" _stage1` -ne 512 ]; then \
		echo >&2 "Check size for stage1 failed"; \
		false;\
	fi

	mv _stage1 stage1

PHONY += clean
clean:
	@rm -f stage1

PHONY += run
run: stage1
	qemu-system-x86_64 -hda stage1

.PHONY:= $(PHONY)
