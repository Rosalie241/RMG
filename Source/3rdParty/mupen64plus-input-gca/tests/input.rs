use mupen64plus_input_gca::adapter::{AdapterState, ControllerState, GcAdapter};
use std::time::{Duration, Instant};

fn all_controller_states<'a>(
    state: &'a AdapterState,
) -> impl Iterator<Item = ControllerState> + 'a {
    (0..4).map(move |i| state.controller_state(i))
}

fn any(state: ControllerState) -> bool {
    const CONTROL_DEADZONE: u8 = 15;
    const CONTROL_SENSITIVITY: u8 = 100;
    const C_DEADZONE: u8 = 15;
    const TRIGGER_THRESHOLD: u8 = 168;
    let (stick_x, stick_y) = state.stick_with_deadzone(CONTROL_DEADZONE, CONTROL_SENSITIVITY);
    let (substick_x, substick_y) = state.substick_with_deadzone(C_DEADZONE);
    state.connected
        && (state.a
            || state.b
            || state.x
            || state.y
            || state.start
            || state.left
            || state.right
            || state.down
            || state.up
            || state.l
            || state.trigger_left > TRIGGER_THRESHOLD
            || state.r
            || state.trigger_right > TRIGGER_THRESHOLD
            || state.z
            || stick_x != 0
            || stick_y != 0
            || substick_x != 0
            || substick_y != 0)
}

#[test]
fn receives_input() {
    const ERR: &str = "make sure the adapter is connected, and press the input(s) you want to test";

    let adapter = GcAdapter::new().expect(ERR);
    let started = Instant::now();

    let mut state = AdapterState::new();
    state.set_buf(adapter.read().unwrap());

    if !(0..4).map(|i| state.is_connected(i)).any(|b| b) {
        eprintln!("no controllers detected, but might be a false negative");
    }

    let mut any_input = false;
    loop {
        if started.elapsed() > Duration::from_secs(10) {
            break;
        }

        state.set_buf(adapter.read().unwrap());
        if let Some((i, _)) = (0..4)
            .map(|i| (i, any(state.controller_state(i))))
            .find(|(_, a)| *a)
        {
            any_input = true;
            println!("Channel {}: {:?}", i, state.controller_state(i));
        }
    }

    assert!(any_input);
}
