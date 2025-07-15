module [Event, Attr]

Event : { type : Str }

Attr a : [
  Color Str,
  OnEvent (Event => a)
]
