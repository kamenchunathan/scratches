app [Msg, on_event, handle!] { pf: platform "../platform/main.roc" }

import pf.Event exposing [Event]
import pf.Effects exposing [print!]

Msg : [
    OnInput Str,
    NoOp,
]

on_event : Event -> Msg
on_event = |event|
    when event is
        { type: "onInput" } -> OnInput "Hello world"
        _ -> NoOp

handle! : Msg => {}
handle! = |msg|
    when msg is
        OnInput val ->
            print! "input ${val}"

        NoOp ->
            print! "NoOp"

