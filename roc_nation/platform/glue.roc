platform "wow" requires { Msg } {
    }
    exposes [Effects, View]
    packages {
    }
    imports []
    provides [
        setup_callback_for_host,
        handle_callback_for_host,
    ]

import View exposing [Attr]


setup_callback_for_host : I32 -> List (Attr U32)
setup_callback_for_host = |_|
    []

handle_callback_for_host : U32 -> {}
handle_callback_for_host = |_|
    {}

