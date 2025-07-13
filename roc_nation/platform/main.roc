platform "wow" requires { Msg } {
        on_event! : Event => Msg,
        handle! : Msg => {},
    }
    exposes [Effects, Event]
    packages {
    }
    imports []
    provides [
        setup_callback_for_host!,
        handle_callback_for_host!,
    ]

import Event exposing [Event]

setup_callback_for_host! : I32 => (Event => Box Msg)
setup_callback_for_host! = |_|
    a = 1
    wrapped! = |e| 
        event = { type: "${e.type} ${Num.to_str a}" }
        Box.box (on_event! event)
    wrapped!

handle_callback_for_host! : Box Msg => {}
handle_callback_for_host! = |boxed_msg|
    msg = Box.unbox boxed_msg
    handle! msg

