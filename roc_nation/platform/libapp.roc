app [Msg, on_event, handle!] { pf: platform "./main.roc" }

import pf.Event exposing [Event]

Msg : {}

on_event : Event -> Msg
on_event = |_| {}

handle! : Msg => {}
handle! = |_| {}

