#[cfg(unix)]
#[allow(non_upper_case_globals)]
#[allow(non_camel_case_types)]
#[allow(non_snake_case)]
#[allow(dead_code)]
#[allow(clippy::all)]
pub mod unix;
#[cfg(unix)]
pub use unix::*;

#[cfg(windows)]
#[allow(non_upper_case_globals)]
#[allow(non_camel_case_types)]
#[allow(non_snake_case)]
#[allow(dead_code)]
#[allow(clippy::all)]
pub mod windows;
#[cfg(windows)]
pub use windows::*;

#[cfg(feature = "m64p_compat")]
#[repr(C)]
#[derive(Debug, Clone, Copy)]
#[allow(non_snake_case)]
pub struct CONTROL_M64P {
    pub Present: std::os::raw::c_int,
    pub RawData: std::os::raw::c_int,
    pub Plugin: std::os::raw::c_int,
    pub Type: std::os::raw::c_int,
}
