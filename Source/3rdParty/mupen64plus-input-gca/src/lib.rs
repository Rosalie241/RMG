#[macro_use]
mod debug;
pub mod adapter;
pub mod config;
mod ffi;
#[macro_use]
mod static_cstr;

use adapter::ADAPTER_STATE;
use config::Config;
use debug::M64Message;
use ffi::*;
use once_cell::sync::OnceCell;
use static_cstr::StaticCStr;
use std::{
    ffi::{c_void, CStr},
    mem::ManuallyDrop,
    os::raw::{c_char, c_int, c_uchar},
    path::Path,
    ptr,
    sync::atomic::{AtomicBool, Ordering},
};

#[cfg(unix)]
use libloading::os::unix::Library;
#[cfg(windows)]
use libloading::os::windows::Library;

struct PluginInfo {
    name: StaticCStr,
    version: c_int,
    target_api_version: c_int,
}

static PLUGIN_INFO: PluginInfo = PluginInfo {
    name: static_cstr!("GC Adapter (for Wii U or Switch) Input Plugin"),
    version: 0x000600,            // v0.6.0
    target_api_version: 0x020100, // v2.1.0
};

static IS_INIT: AtomicBool = AtomicBool::new(false);

static CONFIG: OnceCell<Config> = OnceCell::new();

/// Start up the plugin.
///
/// # Safety
///
/// `core_lib_handle` cannot be null and must be a pointer to the mupen64plus-core dynamic library.
/// `debug_callback` cannot be null and must be a valid C function pointer with the correct type signature.
#[no_mangle]
pub unsafe extern "C" fn PluginStartup(
    core_lib_handle: m64p_dynlib_handle,
    context: *mut c_void,
    debug_callback: debug::DebugCallback,
) -> m64p_error {
    if IS_INIT.load(Ordering::Acquire) {
        debug_print!(M64Message::Error, "Plugin was already initialized");
        return m64p_error_M64ERR_ALREADY_INIT;
    }

    IS_INIT.store(true, Ordering::Release);

    // Register a custom panic hook in order to stop the adapter thread
    let default_panic = std::panic::take_hook();
    std::panic::set_hook(Box::new(move |p| {
        debug_print!(M64Message::Error, "panic occurred");
        IS_INIT.store(false, Ordering::Release);
        default_panic(p);
    }));

    debug::init(debug_callback, context);
    debug_print!(M64Message::Info, "PluginStartup called");

    // Make sure to NOT free the library associated with the handle
    let lib = ManuallyDrop::new(Library::from_raw(core_lib_handle.cast()));

    let core_api_version_fn = if let Ok(sym) =
        lib.get::<extern "C" fn(*mut c_int, *mut c_int, *mut c_int, *mut c_int)>(
            b"CoreGetAPIVersions\0",
        ) {
        sym
    } else {
        debug_print!(
            M64Message::Error,
            "Could not find function for getting core API versions"
        );
        return m64p_error_M64ERR_INPUT_INVALID;
    };

    let mut core_ver = 0;
    core_api_version_fn(
        &mut core_ver as *mut _,
        ptr::null_mut(),
        ptr::null_mut(),
        ptr::null_mut(),
    );

    debug_print!(
        M64Message::Info,
        "Core API reported version {:#08X}",
        core_ver
    );

    if core_ver < PLUGIN_INFO.target_api_version
        || core_ver & 0xfff0000 != PLUGIN_INFO.target_api_version & 0xfff0000
    {
        debug_print!(
            M64Message::Error,
            "Plugin is incompatible with core API version"
        );
        return m64p_error_M64ERR_INCOMPATIBLE;
    }

    adapter::start_read_thread();

    let cfg_file_name = "mupen64plus-input-gca.toml";
    let cfg_path = if let Ok(sym) =
        lib.get::<extern "C" fn() -> *const c_char>(b"ConfigGetUserConfigPath\0")
    {
        let usr_cfg_dir = CStr::from_ptr(sym()).to_string_lossy();
        Path::new(usr_cfg_dir.as_ref()).join(cfg_file_name)
    } else {
        debug_print!(
            M64Message::Error,
            "Could not find function for getting user config path"
        );

        Path::new(cfg_file_name)
            .canonicalize()
            .expect("could not canonicalize relative path")
    };

    debug_print!(
        M64Message::Info,
        "Using user configuration path: {}",
        cfg_path.display()
    );

    // Ignore error if the cell was alredy initialized
    let _ = CONFIG.set(Config::read_from_file(&cfg_path).unwrap_or_else(|e| {
        debug_print!(M64Message::Error, "Config error: {:?}", e);
        Config::create(&cfg_path).unwrap_or_else(|e| e)
    }));

    m64p_error_M64ERR_SUCCESS
}

/// Shut down the plugin.
///
/// This function is not unsafe, but if this is not called then the adapter thread will continue running.
#[no_mangle]
pub extern "C" fn PluginShutdown() -> m64p_error {
    debug_print!(M64Message::Info, "PluginShutdown called");

    IS_INIT.store(false, Ordering::Release);

    m64p_error_M64ERR_SUCCESS
}

/// Get the plugin type, version, target API version, name, and capabilities.
///
/// # Safety
///
/// The caller has to make sure the given pointers are valid.
#[no_mangle]
pub unsafe extern "C" fn PluginGetVersion(
    plugin_type: *mut m64p_plugin_type,
    plugin_version: *mut c_int,
    api_version: *mut c_int,
    plugin_name_ptr: *mut *const c_char,
    capabilities: *mut c_int,
) -> m64p_error {
    debug_print!(M64Message::Info, "PluginGetVersion called");

    if !plugin_type.is_null() {
        *plugin_type = m64p_plugin_type_M64PLUGIN_INPUT;
    }
    if !plugin_version.is_null() {
        *plugin_version = PLUGIN_INFO.version;
    }
    if !api_version.is_null() {
        *api_version = PLUGIN_INFO.target_api_version;
    }
    if !plugin_name_ptr.is_null() {
        *plugin_name_ptr = PLUGIN_INFO.name.as_ptr();
    }
    if !capabilities.is_null() {
        *capabilities = 0;
    }

    m64p_error_M64ERR_SUCCESS
}

/// Initiate controllers by filling the given `CONTROL_INFO` struct.
///
/// # Safety
///
/// `control_info` must point to an initialized `CONTROL_INFO` struct, and the `Controls` field must point to an array
/// of length 4 with initialized `CONTROL` structs.
#[no_mangle]
pub unsafe extern "C" fn InitiateControllers(control_info: CONTROL_INFO) {
    debug_print!(M64Message::Info, "InitiateControllers called");

    let controls = control_info.Controls;
    #[cfg(feature = "m64p_compat")]
    let controls = controls as *mut CONTROL_M64P;

    for i in 0..4 {
        (*controls.add(i)).RawData = 0;
        (*controls.add(i)).Present = 1;
    }

    if !adapter::ADAPTER_STATE.any_connected() {
        debug_print!(
            M64Message::Warning,
            "No controllers connected, but hotplugging is supported"
        );
    }
}

/// Get the state of the buttons by reading from the adapter.
///
/// # Safety
///
/// `keys` must point to an intialized `BUTTONS` union.
#[no_mangle]
pub unsafe extern "C" fn GetKeys(control: c_int, keys: *mut BUTTONS) {
    let s = ADAPTER_STATE.controller_state(control);
    if !s.connected {
        return;
    }

    let keys = &mut *keys;
    keys.Value = 0;

    let cfg = CONFIG.get().unwrap();
    let (stick_x, stick_y) =
        s.stick_with_deadzone(cfg.control_stick_deadzone, cfg.control_stick_sensitivity);
    let (substick_x, substick_y) = s.substick_with_deadzone(cfg.c_stick_deadzone);

    if s.right {
        keys.Value |= cfg.controller_mapping.d_pad_right.bit_pattern();
    }
    if s.left {
        keys.Value |= cfg.controller_mapping.d_pad_left.bit_pattern();
    }
    if s.down {
        keys.Value |= cfg.controller_mapping.d_pad_down.bit_pattern();
    }
    if s.up {
        keys.Value |= cfg.controller_mapping.d_pad_up.bit_pattern();
    }
    if s.start {
        keys.Value |= cfg.controller_mapping.start.bit_pattern();
    }
    if s.a {
        keys.Value |= cfg.controller_mapping.a.bit_pattern();
    }
    if s.b {
        keys.Value |= cfg.controller_mapping.b.bit_pattern();
    }
    if s.x {
        keys.Value |= cfg.controller_mapping.x.bit_pattern();
    }
    if s.y {
        keys.Value |= cfg.controller_mapping.y.bit_pattern();
    }
    if substick_x < 0 {
        keys.Value |= cfg.controller_mapping.c_stick_left.bit_pattern();
    }
    if substick_x > 0 {
        keys.Value |= cfg.controller_mapping.c_stick_right.bit_pattern();
    }
    if substick_y < 0 {
        keys.Value |= cfg.controller_mapping.c_stick_down.bit_pattern();
    }
    if substick_y > 0 {
        keys.Value |= cfg.controller_mapping.c_stick_up.bit_pattern();
    }
    if s.l || s.trigger_left > cfg.trigger_threshold {
        keys.Value |= cfg.controller_mapping.l.bit_pattern();
    }
    if s.r || s.trigger_right > cfg.trigger_threshold {
        keys.Value |= cfg.controller_mapping.r.bit_pattern();
    }
    if s.z {
        keys.Value |= cfg.controller_mapping.z.bit_pattern();
    }

    keys.__bindgen_anon_1.set_X_AXIS(stick_x as i32);
    keys.__bindgen_anon_1.set_Y_AXIS(stick_y as i32);
}

/// Process the command and possibly read the controller. Currently unused, since raw data is disabled.
///
/// # Safety
///
/// `command` must be a valid u8 array with length dependent of the given command.
#[no_mangle]
pub unsafe extern "C" fn ReadController(_control: c_int, _command: *mut u8) {}

/// Currently unused, only needed to be a valid input plugin.
#[no_mangle]
pub extern "C" fn ControllerCommand(_control: c_int, _command: *mut c_uchar) {}

/// Currently unused, only needed to be a valid input plugin.
#[no_mangle]
pub extern "C" fn RomOpen() -> c_int {
    debug_print!(M64Message::Info, "RomOpen called");

    1
}

/// Currently unused, only needed to be a valid input plugin.
#[no_mangle]
pub extern "C" fn RomClosed() {
    debug_print!(M64Message::Info, "RomClosed called");
}

/// Currently unused, only needed to be a valid input plugin.
#[no_mangle]
pub extern "C" fn SDL_KeyDown(_keymod: c_int, _keysym: c_int) {
    debug_print!(M64Message::Info, "SDL_KeyDown called");
}

/// Currently unused, only needed to be a valid input plugin.
#[no_mangle]
pub extern "C" fn SDL_KeyUp(_keymod: c_int, _keysym: c_int) {
    debug_print!(M64Message::Info, "SDL_KeyUp called");
}
