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

setup_callback_for_host! : I32 => List (Event => Box Msg)
setup_callback_for_host! = |_|
    a = { bingo: "Yessir", tango: 56, b: 4 }
    # a = Ok "Hello"
    List.range({start: At 0, end: At 5})
        |> List.map 
            (|_| 
                (|e|
                    event = { type: "${e.type} ${Inspect.to_str a}" }
                    Box.box (on_event! event)
                )
            )
    # wrapped! = |e| 
    #     event = { type: "${e.type} ${Inspect.to_str a}" }
    #     Box.box (on_event! event)
    #         
    # [wrapped!]

handle_callback_for_host! : Box Msg => {}
handle_callback_for_host! = |boxed_msg|
    msg = Box.unbox boxed_msg
    handle! msg

