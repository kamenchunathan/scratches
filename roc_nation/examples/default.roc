app [Msg, attrs, handle!] { pf: platform "../platform/main.roc" }

import pf.View exposing [Attr]
import pf.Effects exposing [print!]

Msg : {}

attrs : I32 -> List (Attr Msg)
attrs =  |_|
    [
        Color "Yellow",
        OnEvent 
            ( |{ type }| 
                print! type
                {}
            )
    ]

handle! : Msg => {}
handle! = |_|
    print! "Callback called"
