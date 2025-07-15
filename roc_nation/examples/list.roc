app [Msg, attrs, handle!] { pf: platform "../platform/main.roc" }

import pf.View exposing [Attr]
import pf.Effects exposing [print!]

Msg : List U8

attrs : I32 -> List (Attr Msg)
attrs = |_| [OnEvent (|_| [1, 1, 2, 3, 5, 8, 13])]

handle! : Msg => {}
handle! = |msg| print! "input ${Inspect.to_str msg}"

