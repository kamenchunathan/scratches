mod roc;

#[no_mangle]
pub extern "C" fn rust_main() -> i32 {
    let captures = roc::call_roc_setup_callback();
    let msg = roc::call_roc_callback(captures);
    roc::call_roc_handle_callback(msg);

    0
}
