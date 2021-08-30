#![no_std]
#![no_main]
#![allow(arithmetic_overflow)]

mod memory;
mod textmode;

use textmode::{make_early_txmbuf, OutputBuffer};
use core::panic::PanicInfo;

#[no_mangle]
pub extern "C" fn kmain() -> ! {
    // just to generate .bss
    static mut asdf : u64 = 0;

    let mut txm = make_early_txmbuf();

    unsafe {
        asdf = 12;
        txm.puts("Kernel successfuly loaded from ELF!");
    }

    loop {}
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}