module Test.Main where

import Prelude

import Effect (Effect)
import Effect.Aff (launchAff_)
import Effect.Class.Console (logShow)
import Parsing (runParserT)
import Sheet.Parser (expressionParser) 

main :: Effect Unit
main = launchAff_ do
  res <- runParserT "SUM(A4)" expressionParser
  logShow res

