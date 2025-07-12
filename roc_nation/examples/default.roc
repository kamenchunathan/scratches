app [Msg, on_event, handle!] { pf: platform "../platform/main.roc" }

import pf.Event exposing [Event]
import pf.Effects exposing [print!]

Msg : {}

on_event : Event -> Msg
on_event = |_| {}

handle! : Msg => {}
handle! = |_|
    print! "Callback called"
