use core::option;

pub type Pid = u64;
pub const PID_KERNEL :Pid = 0;
pub const PAGE_SIZE :usize = 4096;
type VMPage = [u8; PAGE_SIZE];

const PAGING_CHILD_ADDR_SHIFT: u64 = 12;
const PAGING_CHILD_ADDR_MASK: u64 = ((1 << 40) - 1) << PAGING_CHILD_ADDR_SHIFT;

pub enum PagePerm {
    R,
    W,
    X,
}

pub struct ProcMem {
    perm: PagePerm,
    addr: u64,
    size: u64, // Size in pages
}

pub struct Task {
    pid: Pid,
    maps: ProcMem,
}

pub struct PML4 {
    entries: [PML4E; usize::pow(2, 9)],
}

pub struct PML4E {
    value: u64,
}

pub trait PML4EImpl {
    fn is_present(&self)   -> bool;
    fn is_writeable(&self) -> bool;
    fn is_user(&self)      -> bool;
    fn is_pwt(&self)       -> bool; // page write-through
    fn is_pcd(&self)       -> bool; // page-level cache disable
    fn is_accessed(&self)  -> bool;
    fn get_pdpt(&self)     -> Option<&mut PDPTE>;
    fn is_nxe(&self)       -> bool; // execute disable
}

impl PML4EImpl for PML4E {
    fn is_present(&self) -> bool {
        (self.value >> 0) & 1 != 0
    }

    fn is_writeable(&self) -> bool {
        (self.value >> 1) & 1 != 0
    }

    fn is_user(&self) -> bool {
        (self.value >> 2) & 1 != 0
    }

    fn is_pwt(&self) -> bool {
        (self.value >> 3) & 1 != 0
    }

    fn is_pcd(&self) -> bool {
        (self.value >> 4) & 1 != 0
    }

    fn is_accessed(&self) -> bool {
        (self.value >> 5) & 1 != 0
    }

    fn get_pdpt(&self) -> Option<&mut PDPTE> {
        match self.is_present() {
            true => {
                let addr = (self.value & PAGING_CHILD_ADDR_MASK) >> PAGING_CHILD_ADDR_SHIFT;

                unsafe {
                    Some(&mut *(addr as *mut PDPTE))
                }
            },

            false => None
        }
    }

    fn is_nxe(&self) -> bool {
        (self.value >> 63) & 1 != 0
    }
}

pub struct PDPT {
    entries: [PDPTE; usize::pow(2, 9)],
}

pub struct PDPTE {
    value: u64,
}

pub trait PDPTEImpl {
    fn is_present(&self)     -> bool;
    fn is_writeable(&self)   -> bool;
    fn is_user(&self)        -> bool;
    fn is_pwt(&self)         -> bool; // page write-through
    fn is_pcd(&self)         -> bool; // page-level cache disable
    fn is_accessed(&self)    -> bool;
    fn is_mapping_1gb(&self) -> bool;
    fn get_pdpt(&self)       -> Option<&mut PD>;
    fn is_nxe(&self)         -> bool; // execute disable
}

impl PDPTEImpl for PDPTE {
    fn is_present(&self) -> bool {
        (self.value >> 0) & 1 != 0
    }

    fn is_writeable(&self) -> bool {
        (self.value >> 1) & 1 != 0
    }

    fn is_user(&self) -> bool {
        (self.value >> 2) & 1 != 0
    }

    fn is_pwt(&self) -> bool {
        (self.value >> 3) & 1 != 0
    }

    fn is_pcd(&self) -> bool {
        (self.value >> 4) & 1 != 0
    }

    fn is_accessed(&self) -> bool {
        (self.value >> 5) & 1 != 0
    }

    fn is_mapping_1gb(&self) -> bool {
        PAGE_SIZE != 4096
    }

    fn get_pdpt(&self) -> Option<&mut PD> {
        match self.is_present() {
            true => {
                let addr = (self.value & PAGING_CHILD_ADDR_MASK) >> PAGING_CHILD_ADDR_SHIFT;

                unsafe {
                    Some(&mut *(addr as *mut PD))
                }
            },

            false => None
        }
    }

    fn is_nxe(&self) -> bool {
        (self.value >> 63) & 1 != 0
    }
}

pub struct PD {
    entries: [PDE; usize::pow(2, 9)],
}

pub struct PDE {
    value: u64,
}

pub trait PDEImpl {
    fn is_present(&self)     -> bool;
    fn is_writeable(&self)   -> bool;
    fn is_user(&self)        -> bool;
    fn is_pwt(&self)         -> bool; // page write-through
    fn is_pcd(&self)         -> bool; // page-level cache disable
    fn is_accessed(&self)    -> bool;
    fn is_mapping_1gb(&self) -> bool;
    fn get_pdpt(&self)       -> Option<&mut PT>;
    fn is_nxe(&self)         -> bool; // execute disable
}

impl PDEImpl for PDE {
    fn is_present(&self) -> bool {
        (self.value >> 0) & 1 != 0
    }

    fn is_writeable(&self) -> bool {
        (self.value >> 1) & 1 != 0
    }

    fn is_user(&self) -> bool {
        (self.value >> 2) & 1 != 0
    }

    fn is_pwt(&self) -> bool {
        (self.value >> 3) & 1 != 0
    }

    fn is_pcd(&self) -> bool {
        (self.value >> 4) & 1 != 0
    }

    fn is_accessed(&self) -> bool {
        (self.value >> 5) & 1 != 0
    }

    fn is_mapping_1gb(&self) -> bool {
        PAGE_SIZE != 4096
    }

    fn get_pdpt(&self) -> Option<&mut PT> {
        match self.is_present() {
            true => {
                let addr = (self.value & PAGING_CHILD_ADDR_MASK) >> PAGING_CHILD_ADDR_SHIFT;

                unsafe {
                    Some(&mut *(addr as *mut PT))
                }
            },

            false => None
        }
    }

    fn is_nxe(&self) -> bool {
        (self.value >> 63) & 1 != 0
    }
}

pub struct PT {
    entries: [PTE; usize::pow(2, 9)],
}

pub struct PTE {
    value: u64,
}

pub trait PTEImpl {
    fn is_present(&self)     -> bool;
    fn is_writeable(&self)   -> bool;
    fn is_user(&self)        -> bool;
    fn is_pwt(&self)         -> bool; // page write-through
    fn is_pcd(&self)         -> bool; // page-level cache disable
    fn is_accessed(&self)    -> bool;
    fn is_mapping_1gb(&self) -> bool;
    fn get_pdpt(&self)       -> Option<&mut VMPage>;
    fn is_nxe(&self)         -> bool; // execute disable
}

impl PTEImpl for PTE {
    fn is_present(&self) -> bool {
        (self.value >> 0) & 1 != 0
    }

    fn is_writeable(&self) -> bool {
        (self.value >> 1) & 1 != 0
    }

    fn is_user(&self) -> bool {
        (self.value >> 2) & 1 != 0
    }

    fn is_pwt(&self) -> bool {
        (self.value >> 3) & 1 != 0
    }

    fn is_pcd(&self) -> bool {
        (self.value >> 4) & 1 != 0
    }

    fn is_accessed(&self) -> bool {
        (self.value >> 5) & 1 != 0
    }

    fn is_mapping_1gb(&self) -> bool {
        PAGE_SIZE != 4096
    }

    fn get_pdpt(&self) -> Option<&mut VMPage> {
        match self.is_present() {
            true => {
                let addr = (self.value & PAGING_CHILD_ADDR_MASK) >> PAGING_CHILD_ADDR_SHIFT;

                unsafe {
                    Some(&mut *(addr as *mut VMPage))
                }
            },

            false => None
        }
    }

    fn is_nxe(&self) -> bool {
        (self.value >> 63) & 1 != 0
    }
}

