#![no_std]
#![no_main]

use core::panic::PanicInfo;
use core::mem::size_of;
use core::ptr::write_volatile;

const GPUBUF               :u64   = 0xb8000;
const GPUBUF_SCREEN_WIDTH  :usize = 80;
const GPUBUF_SCREEN_HEIGHT :usize = 25;
const GPUBUF_CHAR_MASK     :u16   = 0x0f00;

#[repr(packed)]
struct TxmColor {
    ch: u8,
    bg: u8,
}

struct TxmBuf {
    buf: &'static mut [[TxmColor; GPUBUF_SCREEN_WIDTH]; GPUBUF_SCREEN_HEIGHT],
    curx: usize,
    cury: usize,
}

unsafe fn make_early_txmbuf() -> TxmBuf {
    TxmBuf {
        buf: &mut *(GPUBUF as * mut [[TxmColor; GPUBUF_SCREEN_WIDTH]; GPUBUF_SCREEN_HEIGHT]),
        curx: 0,
        cury: 3,
    }
}

unsafe fn clear_cur_line(buf: &mut TxmBuf) {
    for c in buf.buf[buf.cury].iter_mut() {
        write_volatile::<TxmColor>(&mut *c, TxmColor { ch: ' ' as u8, bg: 0x0f });
    }
}

trait OutputBuffer {
    unsafe fn putc(&mut self, c: char);
    unsafe fn puts(&mut self, s: &str);
    unsafe fn line_feed(&mut self);
}

impl OutputBuffer for TxmBuf {
    unsafe fn putc(&mut self, c: char) {

        write_volatile::<TxmColor>(&mut self.buf[self.cury][self.curx] as *mut TxmColor, TxmColor { ch: c as u8, bg: 0x0f });
        self.buf[self.cury][self.curx] = TxmColor { ch: c as u8, bg: 0x0f };

        self.curx += 1;
        if self.curx >= GPUBUF_SCREEN_WIDTH {
            self.cury += 1;
            if self.cury >= GPUBUF_SCREEN_HEIGHT {
                self.cury = 0;
            }
        }
    }

    unsafe fn puts(&mut self, s: &str) {
        clear_cur_line(self);

        for c in s.chars() {
            self.putc(c);
        }

        self.line_feed();
    }

    unsafe fn line_feed(&mut self) {
            self.curx = 0;
            self.cury += 1;
            if self.cury >= GPUBUF_SCREEN_HEIGHT {
                self.cury = 0;
            }
    }
}

#[no_mangle]
pub extern "C" fn _start() -> ! {
    // just to generate .bss
    static mut asdf : u64 = 0;

    let mut txm = unsafe { make_early_txmbuf() };

    unsafe {
        asdf = 12;
        for i in 0..25 {
            txm.puts("Kernel successfuly loaded from ELF!");
        }
    }

    loop {}
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}