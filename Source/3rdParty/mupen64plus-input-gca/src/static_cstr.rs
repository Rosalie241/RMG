use std::os::raw::c_char;

macro_rules! static_cstr {
    ($s:expr) => {
        StaticCStr(concat!($s, "\0").as_bytes())
    };
}

pub struct StaticCStr(pub &'static [u8]);

impl StaticCStr {
    pub fn as_ptr(&self) -> *const c_char {
        self.0.as_ptr() as *const _
    }
}
