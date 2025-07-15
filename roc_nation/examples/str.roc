app [Msg, attrs, handle!] { pf: platform "../platform/main.roc" }

import pf.View exposing [Attr]
import pf.Effects exposing [print!]

Msg : Str

attrs : I32 -> List (Attr Msg)
attrs = |_|
    [(OnEvent |event|
        when event is
            _ -> "other"
    )]

handle! : Msg => {}
handle! = |msg| print! "input ${msg}"

