module Test.Main where

import Prelude

import Effect (Effect)
import Test.Sheet.Parser as ParserTests
import Test.Sheet.Evaluator as EvaluatorTests
import Test.Spec.Reporter.Console (consoleReporter)
import Test.Spec.Runner.Node (runSpecAndExitProcess)

main :: Effect Unit
main = runSpecAndExitProcess [ consoleReporter ] do
  ParserTests.tests
  EvaluatorTests.tests
