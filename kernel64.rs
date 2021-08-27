#![no_std]
#![no_main]

use core::panic::PanicInfo;

const STAGE_POS: u64 = 0xb8000 + 160 * 3; // 3rd line on screen
const CHAR_MASK: u16 = 0x0f00;

fn puts(s: &str) {
    for (i, c) in s.chars().enumerate() {
        let ch: u16 = CHAR_MASK | c as u16;
        unsafe {
            let gpubuf = (STAGE_POS as * mut u16).add(i);
            *gpubuf = ch;
        }
    }
}

#[no_mangle]
pub extern "C" fn _start() -> ! {
    static mut asdf : u64 = 0;

    unsafe {
        *(0xb8000 as * mut u64) = 0x4141414141414141;
        asdf = 12;
        puts("Kernel successfuly loaded from ELF!");
    }

    loop {}
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}