mod roc;

use roc_app::{roc__setup_callback_for_host_0_caller, setup_callback_for_host};
use roc_std::{ReadOnlyRocList, RocList, RocStr};

extern "C" {

    fn roc__setup_callback_for_host_0_size() -> usize;

}

#[no_mangle]
pub extern "C" fn rust_main() -> i32 {
    // Calculate size

    let event = roc_app::Event {
        r#type: RocStr::from("Bingo"),
    };

    let thunklist: ReadOnlyRocList<()> = setup_callback_for_host(0).into();
    let thunklist: RocList<()> = thunklist.into();
    let elements_ptr = thunklist.as_ptr() as *const u8;
    for i in 0..thunklist.len() {
        unsafe {
            let byte_size = roc__setup_callback_for_host_0_size();
            let closure_data = elements_ptr.add(byte_size * i);
            let mut output = core::mem::MaybeUninit::uninit();
            roc__setup_callback_for_host_0_caller(&event, closure_data, output.as_mut_ptr());
            let msg = output.assume_init();
            roc_app::handle_callback_for_host(msg);
        }
    }
    0
}
