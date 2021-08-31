#![no_std]
#![no_main]
#![allow(arithmetic_overflow)]

mod memory;
mod textmode;
mod x86_64;
mod vm;

use textmode::{make_early_txmbuf, OutputBuffer};
use core::panic::PanicInfo;

#[no_mangle]
pub extern "C" fn kmain(kbase: *const u8, pml4: *mut vm::PML4) -> ! {

    let mut txm = make_early_txmbuf();

    txm.puts("Kernel successfuly loaded from ELF!");

    txm.print("Kernel relocated @ 0x");
    txm.print_hex(kbase as u64);
    txm.line_feed();

    txm.print("PML4 allocated @ 0x");
    txm.print_hex(pml4 as u64);
    txm.line_feed();

    loop {}
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}