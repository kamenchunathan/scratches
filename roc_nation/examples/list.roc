app [Msg, on_event, handle!] { pf: platform "../platform/main.roc" }

import pf.Event exposing [Event]
import pf.Effects exposing [print!]

Msg : List U8

on_event : Event -> Msg
on_event = |_| [1, 1, 2, 3, 5, 8, 13]

handle! : Msg => {}
handle! = |msg| print! "input ${Inspect.to_str msg}"

