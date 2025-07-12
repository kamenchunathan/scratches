app [Msg, on_event, handle!] { pf: platform "../platform/main.roc" }

import pf.Event exposing [Event]
import pf.Effects exposing [print!]

Msg : I32

on_event : Event -> Msg
on_event = |_| 892834

handle! : Msg => {}
handle! = |msg|
    print! "Callback called with msg ${Inspect.to_str msg}"
