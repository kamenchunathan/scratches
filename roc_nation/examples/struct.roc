app [Msg, on_event, handle!] { pf: platform "../platform/main.roc" }

import pf.Event exposing [Event]
import pf.Effects exposing [print!]

Msg : { a : I32, b : Str }

on_event : Event -> Msg
on_event = |_| { a: 0, b: "42069" }

handle! : Msg => {}
handle! = |msg|
    print! "Callback called with msg ${Inspect.to_str msg}"
