platform "wow" requires { Msg } {
        attrs : I32 -> List (Attr Msg),
        handle! : Msg => {},
    }
    exposes [Effects, View]
    packages {
    }
    imports []
    provides [
        setup_callback_for_host!,
        handle_callback_for_host!,
    ]

import View exposing [Attr]

setup_callback_for_host! : I32 => List (Attr (Box Msg))
setup_callback_for_host! = |init|
    attrs init
        |> List.map 
            (|attr| 
                when attr is
                    Color col -> Color col
                    OnEvent on_event! -> OnEvent (|event| Box.box (on_event! event))                           
                
            )

handle_callback_for_host! : Box Msg => {}
handle_callback_for_host! = |boxed_msg|
    msg = Box.unbox boxed_msg
    handle! msg

