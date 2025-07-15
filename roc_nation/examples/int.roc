app [Msg, attrs, handle!] { pf: platform "../platform/main.roc" }

import pf.View exposing [Attr]
import pf.Effects exposing [print!]

Msg : I32

attrs : I32 -> List (Attr Msg)
attrs = |_| [ OnEvent  (|_| 892834)]

handle! : Msg => {}
handle! = |msg|
    print! "Callback called with msg ${Inspect.to_str msg}"
