#![no_std]
#![no_main]

use core::panic::PanicInfo;

mod textmode;
use textmode::{make_early_txmbuf, OutputBuffer};

#[no_mangle]
pub extern "C" fn memset(mut s: * mut u8, c: i32, n: usize) -> u64 {
    let ret = s as u64;

    for _ in 0..n {
        unsafe {
            *s = c as u8;
            s = s.add(1);
        }
    }

    return ret;
}

#[no_mangle]
pub extern "C" fn memcpy(mut dest: * mut u8, mut src: * const u8, n: usize) -> u64 {
    let ret = dest as u64;

    for _ in 0..n {
        unsafe {
            *dest = *src;
            dest = dest.add(1);
            src = src.add(1);
        }
    }

    return ret;
}

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