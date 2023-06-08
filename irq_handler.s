.intel_syntax noprefix

.section .text

.extern isr_default
.extern isr_keyboard

.macro make_exception_handler num
.global exception_handler_\num
exception_handler_\num:
        cli
        push 0
        push \num
        jmp exception_common
.endm

make_exception_handler 0
make_exception_handler 1
make_exception_handler 2
make_exception_handler 3
make_exception_handler 4
make_exception_handler 5
make_exception_handler 6
make_exception_handler 7
make_exception_handler 8
make_exception_handler 9
make_exception_handler 10
make_exception_handler 11
make_exception_handler 12
make_exception_handler 13
make_exception_handler 14
make_exception_handler 15
make_exception_handler 16
make_exception_handler 17
make_exception_handler 18
make_exception_handler 19
make_exception_handler 20
make_exception_handler 21

exception_common:
        push rdi
        push rax
        push rcx
        push rdx
        push r8
        push r9
        push r10
        push r11

        lea rdi, [rsp]
        #call global_exception_handler
        call isr_default

        pop r11
        pop r10
        pop r9
        pop r8
        pop rdx
        pop rcx
        pop rax
        pop rdi

        # Pop error code and exception number
        add rsp, 16

        iretq

.global irq_handler_keyboard
irq_handler_keyboard:
        push 0
        push 0

        push rdi
        push rax
        push rcx
        push rdx
        push r8
        push r9
        push r10
        push r11

        lea rdi, [rsp]
        call isr_keyboard

        pop r11
        pop r10
        pop r9
        pop r8
        pop rdx
        pop rcx
        pop rax
        pop rdi

        # Pop error code and exception number
        add rsp, 16

        iretq

.global irq_handler
irq_handler:
        push 0
        push 0

        push rdi
        push rax
        push rcx
        push rdx
        push r8
        push r9
        push r10
        push r11

        lea rdi, [rsp]
        call isr_default

        pop r11
        pop r10
        pop r9
        pop r8
        pop rdx
        pop rcx
        pop rax
        pop rdi

        # Pop error code and exception number
        add rsp, 16

        iretq

.global trap_handler
trap_handler:
        cli
        hlt
        iretq

