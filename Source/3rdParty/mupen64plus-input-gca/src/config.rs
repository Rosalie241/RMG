use serde::{Deserialize, Serialize};
use std::{
    fs::File,
    io::{self, Read, Write},
    path::Path,
};

#[derive(Debug, Deserialize, Serialize)]
pub struct Config {
    pub control_stick_deadzone: u8,
    pub control_stick_sensitivity: u8,
    pub c_stick_deadzone: u8,
    pub trigger_threshold: u8,
    pub controller_mapping: ControllerMapping,
}

#[derive(Debug, Deserialize, Serialize)]
pub struct ControllerMapping {
    pub a: N64Button,
    pub b: N64Button,
    pub x: N64Button,
    pub y: N64Button,
    pub start: N64Button,
    pub z: N64Button,
    pub l: N64Button,
    pub r: N64Button,
    pub d_pad_left: N64Button,
    pub d_pad_right: N64Button,
    pub d_pad_down: N64Button,
    pub d_pad_up: N64Button,
    pub c_stick_left: N64Button,
    pub c_stick_right: N64Button,
    pub c_stick_down: N64Button,
    pub c_stick_up: N64Button,
}

impl Config {
    pub fn read_from_file<P: AsRef<Path>>(path: P) -> io::Result<Self> {
        let path = path.as_ref();
        let mut file = File::open(path)?;
        let mut string = String::new();
        file.read_to_string(&mut string)?;
        let cfg = toml::from_str(&string).map_err(|e| io::Error::new(io::ErrorKind::Other, e))?;

        Ok(cfg)
    }

    pub fn create<P: AsRef<Path>>(path: P) -> Result<Self, Self> {
        let contents = include_str!("../mupen64plus-input-gca.toml");
        let cfg = toml::from_str(contents).unwrap();

        let path = path.as_ref();
        let mut file = match File::create(path) {
            Ok(f) => f,
            Err(_) => return Err(cfg),
        };

        match file.write_all(contents.as_bytes()) {
            Ok(_) => Ok(cfg),
            Err(_) => Err(cfg),
        }
    }
}

#[derive(Debug, Serialize, Deserialize)]
pub enum N64Button {
    A,
    B,
    Start,
    Z,
    L,
    R,
    DPadLeft,
    DPadRight,
    DPadDown,
    DPadUp,
    CLeft,
    CRight,
    CDown,
    CUp,
    None,
}

impl N64Button {
    pub fn bit_pattern(&self) -> u32 {
        match self {
            N64Button::A => 0x0080,
            N64Button::B => 0x0040,
            N64Button::DPadLeft => 0x0002,
            N64Button::DPadRight => 0x0001,
            N64Button::DPadDown => 0x0004,
            N64Button::DPadUp => 0x0008,
            N64Button::Start => 0x0010,
            N64Button::Z => 0x0020,
            N64Button::R => 0x1000,
            N64Button::L => 0x2000,
            N64Button::CLeft => 0x0200,
            N64Button::CRight => 0x0100,
            N64Button::CDown => 0x0400,
            N64Button::CUp => 0x0800,
            N64Button::None => 0x0000,
        }
    }
}
