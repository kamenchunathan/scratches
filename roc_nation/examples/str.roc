app [Msg, on_event, handle!] { pf: platform "../platform/main.roc" }

import pf.Event exposing [Event]
import pf.Effects exposing [print!]

Msg : Str

on_event : Event -> Msg
on_event = |event|
    when event is
        _ -> "other"

handle! : Msg => {}
handle! = |msg| print! "input ${msg}"

