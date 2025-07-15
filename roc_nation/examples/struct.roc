app [Msg, attrs, handle!] { pf: platform "../platform/main.roc" }

import pf.View exposing [Attr]
import pf.Effects exposing [print!]

Msg : { a : I32, b : Str }

attrs : I32 -> List (Attr Msg)
attrs  = |_| [OnEvent |_| { a: 0, b: "42069" } ]

handle! : Msg => {}
handle! = |msg|
    print! "Callback called with msg ${Inspect.to_str msg}"
