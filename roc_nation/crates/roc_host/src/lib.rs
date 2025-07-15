use roc_app::{setup_callback_for_host, Event};
use roc_std::RocStr;

mod roc;

#[no_mangle]
pub extern "C" fn rust_main() -> i32 {
    // Calculate size

    let event = Event {
        r#type: RocStr::from("Bingo"),
    };

    let thunklist = setup_callback_for_host(0);

    for attr in thunklist.into_iter() {
        let discriminant = attr.discriminant();
        match discriminant {
            roc_app::discriminant_Attr::OnEvent => {
                // NOTE: Won't print if this is not here
                println!("bingo {:?}", discriminant);

                let msg = attr.borrow_mut_OnEvent().force_thunk(event.clone());
                roc_app::handle_callback_for_host(msg);
            }

            roc_app::discriminant_Attr::Color => {}
        }
    }

    std::mem::forget(thunklist);
    0
}
