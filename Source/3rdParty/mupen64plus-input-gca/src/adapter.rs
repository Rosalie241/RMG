use crate::{M64Message, IS_INIT};
use rusb::{DeviceHandle, GlobalContext};
use std::{
    convert::{TryFrom, TryInto},
    fmt::Debug,
    sync::{atomic::Ordering, Mutex},
    thread,
    time::Duration,
};

const ENDPOINT_IN: u8 = 0x81;
const ENDPOINT_OUT: u8 = 0x02;
const READ_LEN: usize = 37;

pub static ADAPTER_STATE: AdapterState = AdapterState::new();

pub fn start_read_thread() {
    thread::spawn(move || {
        debug_print!(M64Message::Info, "Adapter thread started");
        debug_print!(M64Message::Info, "Trying to connect to GameCube adapter...");

        let mut gc_adapter = GcAdapter::blocking_connect();

        debug_print!(M64Message::Info, "Found a GameCube adapter");

        while IS_INIT.load(Ordering::Acquire) {
            match gc_adapter.read() {
                Ok(buf) => *ADAPTER_STATE.buf.lock().unwrap() = buf,
                Err(rusb::Error::NoDevice) => {
                    debug_print!(
                        M64Message::Info,
                        "Adapter disconnected, trying to reconnect..."
                    );
                    gc_adapter = GcAdapter::blocking_connect();
                    debug_print!(M64Message::Info, "Adapter reconnected");
                }
                Err(e) => panic!("error while reading from adapter: {:?}", e),
            }

            // Gives a polling rate of approx. 1000 Hz
            thread::park_timeout(Duration::from_millis(1));
        }

        debug_print!(M64Message::Info, "Adapter thread stopped");
    });
}

pub struct GcAdapter {
    handle: DeviceHandle<GlobalContext>,
}

impl Debug for GcAdapter {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(
            f,
            "GCAdapter with product string: {}",
            self.handle
                .read_product_string_ascii(&self.handle.device().device_descriptor().unwrap())
                .unwrap()
        )
    }
}

impl GcAdapter {
    pub fn new() -> Result<Self, rusb::Error> {
        let device = rusb::devices()?
            .iter()
            .find(|dev| {
                let dev_desc = dev.device_descriptor().unwrap();
                dev_desc.vendor_id() == 0x057E && dev_desc.product_id() == 0x0337
            })
            .ok_or(rusb::Error::NoDevice)?;

        let mut handle = device.open()?;

        if handle.kernel_driver_active(0).unwrap_or(false) {
            handle.detach_kernel_driver(0)?;
        }

        // From Dolphin emulator source:
        // "This call makes Nyko-brand (and perhaps other) adapters work.
        // However it returns LIBUSB_ERROR_PIPE with Mayflash adapters."
        let res = handle.write_control(0x21, 11, 0x0001, 0, &[], Duration::from_millis(1000));
        if let Err(e) = res {
            debug_print!(
                M64Message::Warning,
                "Control transfer failed with error: {:?}",
                e
            );
        }

        handle.claim_interface(0)?;
        handle.write_interrupt(ENDPOINT_OUT, &[0x13], Duration::from_millis(16))?;

        Ok(GcAdapter { handle })
    }

    /// Continuously try to connect to the adapter
    pub fn blocking_connect() -> Self {
        loop {
            if let Ok(gc) = GcAdapter::new() {
                break gc;
            }

            thread::park_timeout(Duration::from_secs(1));
        }
    }

    pub fn read(&self) -> rusb::Result<[u8; READ_LEN]> {
        let mut buf = [0; READ_LEN];

        match self
            .handle
            .read_interrupt(ENDPOINT_IN, &mut buf, Duration::from_millis(16))
        {
            Ok(_) | Err(rusb::Error::Timeout) => Ok(buf),
            Err(e) => Err(e),
        }
    }
}

#[derive(Debug)]
pub struct AdapterState {
    buf: Mutex<[u8; 37]>,
}

impl AdapterState {
    pub const fn new() -> Self {
        AdapterState {
            buf: Mutex::new([0; READ_LEN]),
        }
    }

    /// Get the `ControllerState` for the given channel
    pub fn controller_state<T>(&self, channel: T) -> ControllerState
    where
        T: TryInto<Channel>,
        <T as TryInto<Channel>>::Error: Debug,
    {
        let channel = channel.try_into().unwrap() as usize;
        let buf = *self.buf.lock().unwrap();

        if let [status, b1, b2, stick_x, stick_y, substick_x, substick_y, trigger_left, trigger_right, ..] =
            buf[(9 * channel) + 1..]
        {
            ControllerState {
                connected: is_controller_connected(status),

                a: b1 & (1 << 0) > 0,
                b: b1 & (1 << 1) > 0,
                x: b1 & (1 << 2) > 0,
                y: b1 & (1 << 3) > 0,

                left: b1 & (1 << 4) > 0,
                right: b1 & (1 << 5) > 0,
                down: b1 & (1 << 6) > 0,
                up: b1 & (1 << 7) > 0,

                start: b2 & (1 << 0) > 0,
                z: b2 & (1 << 1) > 0,
                r: b2 & (1 << 2) > 0,
                l: b2 & (1 << 3) > 0,

                stick_x,
                stick_y,
                substick_x,
                substick_y,
                trigger_left,
                trigger_right,
            }
        } else {
            unreachable!();
        }
    }

    /// Check if a controller is connected to the given channel.
    pub fn is_connected<T>(&self, channel: T) -> bool
    where
        T: TryInto<Channel>,
        <T as TryInto<Channel>>::Error: Debug,
    {
        let buf = *self.buf.lock().unwrap();
        let channel = channel.try_into().unwrap();

        let status = buf[1 + (9 * channel as usize)] >> 4;
        is_controller_connected(status)
    }

    pub fn any_connected(&self) -> bool {
        (0..4)
            .map(|i| self.is_connected(i))
            .any(std::convert::identity)
    }

    pub fn set_buf(&mut self, buf: [u8; 37]) {
        *self.buf.get_mut().unwrap() = buf;
    }
}

impl Default for AdapterState {
    fn default() -> Self {
        AdapterState::new()
    }
}

#[derive(Debug, Default, Copy, Clone)]
pub struct ControllerState {
    pub connected: bool,

    pub a: bool,
    pub b: bool,
    pub x: bool,
    pub y: bool,

    pub left: bool,
    pub right: bool,
    pub down: bool,
    pub up: bool,

    pub start: bool,
    pub z: bool,
    pub r: bool,
    pub l: bool,

    pub stick_x: u8,
    pub stick_y: u8,
    pub substick_x: u8,
    pub substick_y: u8,
    pub trigger_left: u8,
    pub trigger_right: u8,
}

impl ControllerState {
    pub fn stick_with_deadzone(&self, deadzone: u8, sensitivity: u8) -> (i8, i8) {
        const STICK_MAX: i32 = i8::MAX as i32;

        let x = self.stick_x.wrapping_add(128) as i8;
        let y = self.stick_y.wrapping_add(128) as i8;

        // Convert cartesian coordinates to polar coordinates (radius)
        let radius = ((x as f32).powi(2) + (y as f32).powi(2)).sqrt();

        if radius <= deadzone as f32 {
            return (0, 0);
        }

        // Convert cartesian coordinates to polar coordinates (angle)
        let angle = (y as f32).atan2(x as f32);

        let deadzone = deadzone as i32;
        // User-facing sensitivity is inverted (so that higher values give higher radius)
        let sensitivity = u8::MAX as i32 - sensitivity as i32;

        // Scale radius to counteract the deadzone, and fit the radius to the range [-80, 80] (N64
        // stick range).
        // This formula is a simplified version of the following:
        //
        // let radius = (radius - deadzone as f32) * (STICK_MAX as f32 / (STICK_MAX - deadzone) as f32);
        // let radius = radius * 80.0 / (STICK_MAX as f32 * (sensitivity as f32 / 100.0)) as f32;
        let radius =
            8000.0 * (radius - deadzone as f32) / (sensitivity * (STICK_MAX - deadzone)) as f32;

        // Convert back to cartesian coordinates
        let x = (radius * angle.cos()).round() as i8;
        let y = (radius * angle.sin()).round() as i8;

        (x, y)
    }

    pub fn substick_with_deadzone(&self, deadzone: u8) -> (i8, i8) {
        let x = self.substick_x.wrapping_add(128) as i8;
        let y = self.substick_y.wrapping_add(128) as i8;

        let x = if x.unsigned_abs() < deadzone { 0 } else { x };

        let y = if y.unsigned_abs() < deadzone { 0 } else { y };

        (x, y)
    }
}

fn is_controller_connected(status: u8) -> bool {
    // 0x10 = Normal
    // 0x20 = Wavebird
    let controller_type = status & (0x10 | 0x20);
    controller_type == 0x10 || controller_type == 0x20
}

#[derive(Debug, Copy, Clone)]
pub enum Channel {
    One = 0,
    Two = 1,
    Three = 2,
    Four = 3,
}

impl TryFrom<usize> for Channel {
    type Error = usize;

    fn try_from(val: usize) -> Result<Self, Self::Error> {
        match val {
            0 => Ok(Channel::One),
            1 => Ok(Channel::Two),
            2 => Ok(Channel::Three),
            3 => Ok(Channel::Four),
            x => Err(x),
        }
    }
}

impl TryFrom<i32> for Channel {
    type Error = i32;

    fn try_from(val: i32) -> Result<Self, Self::Error> {
        match val {
            0 => Ok(Channel::One),
            1 => Ok(Channel::Two),
            2 => Ok(Channel::Three),
            3 => Ok(Channel::Four),
            x => Err(x),
        }
    }
}
