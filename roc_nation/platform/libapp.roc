app [Msg, attrs, handle!] { pf: platform "./main.roc" }

import pf.View exposing [Attr]

Msg : {}

attrs : I32 -> List (Attr Msg)
attrs = |_| []

handle! : Msg => {}
handle! = |_| {}

