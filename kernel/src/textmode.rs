use core::ptr::write_volatile;

// This should probably go out of TextMode module at some point
pub trait OutputBuffer {
    fn putc(&mut self, c: char);
    fn puts(&mut self, s: &str);
    fn line_feed(&mut self);
}

trait TextModeBuffer {
    fn clear_cur_line(&mut self);
}

const GPUBUF               :u64   = 0xb8000;
const GPUBUF_SCREEN_WIDTH  :usize = 80;
const GPUBUF_SCREEN_HEIGHT :usize = 25;
const GPUBUF_CHAR_BG_BLACK :u8    = 0x0f;

#[repr(packed)]

// Disbale warning that the field is never read.
// This type is a view over VGA text mode buffer.
#[allow(dead_code)]

pub struct TxmColor {
    ch: u8,
    bg: u8,
}

pub struct TxmBuf {
    buf: &'static mut [[TxmColor; GPUBUF_SCREEN_WIDTH]; GPUBUF_SCREEN_HEIGHT],
    curx: usize,
    cury: usize,
}

impl TextModeBuffer for TxmBuf {
    fn clear_cur_line(&mut self) {
        for (i, c) in self.buf[self.cury].iter_mut().enumerate() {

            if i >= GPUBUF_SCREEN_WIDTH {
                break;
            }

            unsafe {
                write_volatile::<TxmColor>(c, TxmColor { ch: ' ' as u8, bg: GPUBUF_CHAR_BG_BLACK });
            }
        }
    }
}

impl OutputBuffer for TxmBuf {
    fn putc(&mut self, c: char) {
        assert!(self.cury < GPUBUF_SCREEN_HEIGHT);
        assert!(self.curx < GPUBUF_SCREEN_WIDTH);

        unsafe {
            write_volatile::<TxmColor>(&mut self.buf[self.cury][self.curx] as *mut TxmColor, TxmColor { ch: c as u8, bg: 0x0f });
        }

        self.curx += 1;
        if self.curx >= GPUBUF_SCREEN_WIDTH {
            self.curx = 0;
            self.cury += 1;
            if self.cury >= GPUBUF_SCREEN_HEIGHT {
                self.cury = 0;
            }
        }
    }

    fn puts(&mut self, s: &str) {

        self.clear_cur_line();

        for c in s.chars() {
            self.putc(c);
        }

        self.line_feed();
    }

    fn line_feed(&mut self) {
        self.curx = 0;
        self.cury += 1;
        if self.cury >= GPUBUF_SCREEN_HEIGHT {
            self.cury = 0;
        }
    }
}

pub fn make_early_txmbuf() -> TxmBuf {
    TxmBuf {
        buf: unsafe { &mut *(GPUBUF as * mut [[TxmColor; GPUBUF_SCREEN_WIDTH]; GPUBUF_SCREEN_HEIGHT]) },
        curx: 0,
        cury: 3,
    }
}

