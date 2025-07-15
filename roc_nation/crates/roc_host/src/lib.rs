use roc_app::{setup_callback_for_host, Event};
use roc_std::RocStr;

mod roc;

#[no_mangle]
pub extern "C" fn rust_main() -> i32 {
    // Calculate size

    let event = Event {
        r#type: RocStr::from("Bingo"),
    };

    let mut attrs = setup_callback_for_host(0);

    attrs.for_each(|attr| {
        let discriminant = attr.discriminant();
        println!("wow {:?}", discriminant);
        match discriminant {
            roc_app::discriminant_Attr::OnEvent => {
                let msg = attr.borrow_mut_OnEvent().force_thunk(event.clone());
                roc_app::handle_callback_for_host(msg);
            }

            roc_app::discriminant_Attr::Color => {
                println!("Color {:?}", attr.borrow_Color());
            }
        }
    });

    std::mem::forget(attrs);
    0
}
