app [Msg, attrs, handle!] { pf: platform "../platform/main.roc" }

import pf.View exposing [Attr]
import pf.Effects exposing [print!]

Msg : [
    OnInput Str,
    NoOp,
]

attrs : I32 -> List (Attr Msg)
attrs  =  |_| [
    OnEvent (|event|
        when event is
            { type: "onInput" } -> OnInput "Hello world"
            _ -> NoOp
        )
    ]

handle! : Msg => {}
handle! = |msg|
    when msg is
        OnInput val ->
            print! "input ${val}"

        NoOp ->
            print! "NoOp"

