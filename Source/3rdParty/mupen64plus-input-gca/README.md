# mupen64plus-input-gca

An input plugin for Mupen64Plus using the GameCube controller adapter for Wii U and Switch.

## Installing

**Note:** There are currently only builds for 64-bit Mupen64Plus.

1. Follow the first section of [this Dolphin guide](https://dolphin-emu.org/docs/guides/how-use-official-gc-controller-adapter-wii-u#Installation) to set up your adapter (use Zadig if you are on Windows).

1. Download the plugin ZIP from [the latest release](../../releases/latest).

1. Extract the plugin into your Mupen64Plus folder.

## Usage

Select the plugin in your Mupen64Plus frontend and connect your adapter, and you are good to go!

## Configuration

After the plugin has been used at least once, a file will be generated in your Mupen64Plus user configuration folder named
`mupen64plus-input-gca.toml`. The default location of the Mupen64Plus user configuration folder is

* `C:\Users\<username>\AppData\Roaming\Mupen64Plus\`, on Windows
* `$XDG_CONFIG_HOME/mupen64plus/` or `$HOME/.config/mupen64plus/`, on Linux
* `$HOME/Library/Application Support/Mupen64Plus/`, on macOS.

This file contains the configuration for the plugin. Changes in the configuration will be
loaded the next time you start your frontend.

You can configure the following:

* Deadzones for the control stick and C-stick.
* Control stick sensitivity.
    * Note that the default sensitivity setting is based on the controllers I had available when testing. All GameCube
    controllers are different, so you might need to change this to something that works for you. Having the sensitivity
    too low will prevent you from reaching the maximum input with the stick, and having it too high will make it
    difficult to control.
* Threshold for the trigger buttons (L and R).
    * This controls how far the triggers need to be pressed before an input is registered.
* The controller mapping between the GameCube controller and the N64 buttons.

The default controller mapping is what you would expect, except for:

* Y is C-button left
* X is C-button right
* L and Z are swapped (GC L is N64 Z and GC Z is N64 L)

## Building

Build requirements:

* Cargo

To build the project:

```
$ git clone https://github.com/amatho/mupen64plus-input-gca
$ cd mupen64plus-input-gca
$ cargo build --release
```

The compiled plugin will be at `target/release/mupen64plus_input_gca.(dll|dylib|so)`.

**Note:** The compiled dynamic library will have underscores in it's name, but m64p (linked above) will only look for plugins with hyphens. Just rename the file and m64p will find it.

`bindgen` is used to generate the Rust bindings for the Mupen64Plus API found in `src/ffi/`. See [the `bindgen` User Guide](https://rust-lang.github.io/rust-bindgen/command-line-usage.html)
for installation instructions, and [this section of the guide](https://rust-lang.github.io/rust-bindgen/requirements.html) for requirements for running `bindgen`.

## Contributing

Feel free to open issues or pull requests.

## License

Licensed under the MIT license, see [LICENSE](LICENSE). For external code in `extern` (headers from the Mupen64Plus-Core API), see [LICENSES](extern/LICENSES).
