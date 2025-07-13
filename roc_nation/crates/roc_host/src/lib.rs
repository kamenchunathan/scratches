use roc_std::RocStr;

mod roc;

#[no_mangle]
pub extern "C" fn rust_main() -> i32 {
    let thunk = roc_app::setup_callback_for_host(0);
    let event = roc_app::Event {
        r#type: RocStr::from("click"),
    };
    let msg = thunk.force_thunk(event);
    roc_app::handle_callback_for_host(msg);
    // let captures = roc::call_roc_setup_callback();
    // let msg = roc::call_roc_callback(captures);
    // roc::call_roc_handle_callback(msg);

    0
}
