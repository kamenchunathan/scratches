module Main
  ( main
  ) where

import Prelude

import App as App
import Effect (Effect)
import Effect.Class.Console (log)
import Halogen.Aff (awaitBody)
import Halogen.Aff as Halogen.Aff
import Halogen.VDom.Driver (runUI)

main :: Effect Unit
main = Halogen.Aff.runHalogenAff do
  log "Starting app..."
  body <- awaitBody
  runUI App.component unit body

