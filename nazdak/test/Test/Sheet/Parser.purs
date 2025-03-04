module Test.Sheet.Parser where

import Prelude

import Sheet.Index (CellIndex(..))
import Sheet.AST (Expr(..), Ident(..), Literal(..), Operator(..))
import Data.Either (Either(..))
import Parsing (runParser)
import Sheet.Parser
  ( argListParser
  , binOpParser
  , cellIndex
  , cellRefParser
  , expressionParser
  , formulaParser
  , functionParser
  , ident
  , literalParser
  , operatorParser
  , posInt
  )
import Test.Spec (Spec, describe, it)
import Test.Spec.Assertions (fail, shouldEqual)

tests :: Spec Unit
tests = describe "Sheet.Parser" do
  describe "posInt" do
    it "parses positive integers" do
      let result = runParser "123" posInt
      result `shouldEqual` Right 123

    it "fails on non-integers" do
      let result = runParser "abc" posInt
      case result of
        Left _ -> pure unit
        Right val -> fail $ "Should have failed but got: " <> show val

  describe "cellIndex" do
    it "parses cell indices with one letter" do
      let result = runParser "A1" cellIndex
      result `shouldEqual` Right (CellIndex 1 "A")

    it "parses cell indices with two letters" do
      let result = runParser "AB42" cellIndex
      result `shouldEqual` Right (CellIndex 42 "AB")

    it "fails on invalid cell indices" do
      let result = runParser "123" cellIndex
      case result of
        Left _ -> pure unit
        Right val -> fail $ "Should have failed but got: " <> show val

  describe "ident" do
    it "parses uppercase identifiers" do
      let result = runParser "SUM" ident
      result `shouldEqual` Right (Ident "SUM")

  describe "operatorParser" do
    it "parses addition operator" do
      let result = runParser "+" operatorParser
      result `shouldEqual` Right Add

    it "parses subtraction operator" do
      let result = runParser "-" operatorParser
      result `shouldEqual` Right Sub

    it "parses multiplication operator" do
      let result = runParser "*" operatorParser
      result `shouldEqual` Right Mul

    it "parses division operator" do
      let result = runParser "/" operatorParser
      result `shouldEqual` Right Div

    it "fails on invalid operators" do
      let result = runParser "^" operatorParser
      case result of
        Left _ -> pure unit
        Right val -> fail $ "Should have failed but got: " <> show val

  describe "literalParser" do
    it "parses numeric literals" do
      let result = runParser "123" literalParser
      result `shouldEqual` Right (Literal (NumLit 123.0))

    it "parses decimal literals" do
      let result = runParser "123.45" literalParser
      result `shouldEqual` Right (Literal (NumLit 123.45))

    it "parses string literals" do
      let result = runParser "abc123" literalParser
      result `shouldEqual` Right (Literal (StrLit "abc123"))

  describe "cellRefParser" do
    it "parses cell references" do
      let result = runParser "A1" cellRefParser
      result `shouldEqual` Right (CellRef (CellIndex 1 "A"))

    it "parses cell references with two-letter columns" do
      let result = runParser "AB42" cellRefParser
      result `shouldEqual` Right (CellRef (CellIndex 42 "AB"))

  describe "functionParser" do
    it "parses simple functions without arguments" do
      let result = runParser "SUM()" functionParser
      result `shouldEqual` Right (Function (Ident "SUM") [])

    it "parses functions with a single literal argument" do
      let result = runParser "SUM(123)" functionParser
      result `shouldEqual` Right (Function (Ident "SUM") [ Literal (NumLit 123.0) ])

    it "parses functions with multiple arguments" do
      let result = runParser "SUM(123,456)" functionParser
      result `shouldEqual` Right
        ( Function (Ident "SUM")
            [ Literal (NumLit 123.0)
            , Literal (NumLit 456.0)
            ]
        )

    it "parses functions with cell reference arguments" do
      let result = runParser "SUM(A1,B2)" functionParser
      result `shouldEqual` Right
        ( Function (Ident "SUM")
            [ CellRef (CellIndex 1 "A")
            , CellRef (CellIndex 2 "B")
            ]
        )

  describe "binOpParser" do
    it "parses simple addition" do
      let result = runParser "A1+B2" binOpParser
      result `shouldEqual` Right
        ( BinOp Add
            (CellRef (CellIndex 1 "A"))
            (CellRef (CellIndex 2 "B"))
        )

    it "parses simple subtraction" do
      let result = runParser "A1-B2" binOpParser
      result `shouldEqual` Right
        ( BinOp Sub
            (CellRef (CellIndex 1 "A"))
            (CellRef (CellIndex 2 "B"))
        )

    it "parses simple multiplication" do
      let result = runParser "A1*B2" binOpParser
      result `shouldEqual` Right
        ( BinOp Mul
            (CellRef (CellIndex 1 "A"))
            (CellRef (CellIndex 2 "B"))
        )

    it "parses simple division" do
      let result = runParser "A1/B2" binOpParser
      result `shouldEqual` Right
        ( BinOp Div
            (CellRef (CellIndex 1 "A"))
            (CellRef (CellIndex 2 "B"))
        )

  describe "expressionParser" do
    it "parses literals" do
      let result = runParser "123" expressionParser
      result `shouldEqual` Right (Literal (NumLit 123.0))

    it "parses cell references" do
      let result = runParser "A1" expressionParser
      result `shouldEqual` Right (CellRef (CellIndex 1 "A"))

    it "parses functions" do
      let result = runParser "SUM(A1,B2)" expressionParser
      result `shouldEqual` Right
        ( Function (Ident "SUM")
            [ CellRef (CellIndex 1 "A")
            , CellRef (CellIndex 2 "B")
            ]
        )

    it "parses binary operations" do
      let result = runParser "A1+B2" expressionParser
      result `shouldEqual` Right
        ( BinOp Add
            (CellRef (CellIndex 1 "A"))
            (CellRef (CellIndex 2 "B"))
        )

    it "parses complex nested expressions" do
      let result = runParser "SUM(A1+B2,C3*D4)" expressionParser
      result `shouldEqual` Right
        ( Function (Ident "SUM")
            [ BinOp Add
                (CellRef (CellIndex 1 "A"))
                (CellRef (CellIndex 2 "B"))
            , BinOp Mul
                (CellRef (CellIndex 3 "C"))
                (CellRef (CellIndex 4 "D"))
            ]
        )

    it "parses complex binary operations" do
      let result = runParser "A1+B2*C3" expressionParser
      result `shouldEqual` Right
        ( BinOp Add
            (CellRef (CellIndex 1 "A"))
            ( BinOp Mul
                (CellRef (CellIndex 2 "B"))
                (CellRef (CellIndex 3 "C"))
            )
        )

  describe "argListParser" do
    it "parses empty argument list" do
      let result = runParser "" argListParser
      result `shouldEqual` Right []

    it "parses single argument" do
      let result = runParser "123" argListParser
      result `shouldEqual` Right [ Literal (NumLit 123.0) ]

    it "parses multiple arguments" do
      let result = runParser "A1,B2,123" argListParser
      result `shouldEqual` Right
        [ CellRef (CellIndex 1 "A")
        , CellRef (CellIndex 2 "B")
        , Literal (NumLit 123.0)
        ]

    it "parses complex arguments" do
      let result = runParser "A1+B2,C3*D4" argListParser
      result `shouldEqual` Right
        [ BinOp Add
            (CellRef (CellIndex 1 "A"))
            (CellRef (CellIndex 2 "B"))
        , BinOp Mul
            (CellRef (CellIndex 3 "C"))
            (CellRef (CellIndex 4 "D"))
        ]

  describe "Sheet.Parser.formulaParser" do
    it "parses a simple numeric literal formula" do
      let input = "=123"
      let expected = Literal (NumLit 123.0)
      runParser input formulaParser `shouldEqual` Right expected

    it "parses a simple string literal formula" do
      let input = "=abc123"
      let expected = Literal (StrLit "abc123")
      runParser input formulaParser `shouldEqual` Right expected

    it "parses a cell reference formula" do
      let input = "=A1"
      let expected = CellRef (CellIndex 1 "A")
      runParser input formulaParser `shouldEqual` Right expected

    it "parses a cell reference with double letter column" do
      let input = "=AB42"
      let expected = CellRef (CellIndex 42 "AB")
      runParser input formulaParser `shouldEqual` Right expected

    it "parses a simple addition operation" do
      let input = "=A1+123"
      let expected = BinOp Add (CellRef (CellIndex 1 "A")) (Literal (NumLit 123.0))
      runParser input formulaParser `shouldEqual` Right expected

    it "parses a simple subtraction operation" do
      let input = "=B2-42"
      let expected = BinOp Sub (CellRef (CellIndex 2 "B")) (Literal (NumLit 42.0))
      runParser input formulaParser `shouldEqual` Right expected

    it "parses a simple multiplication operation" do
      let input = "=C3*10"
      let expected = BinOp Mul (CellRef (CellIndex 3 "C")) (Literal (NumLit 10.0))
      runParser input formulaParser `shouldEqual` Right expected

    it "parses a simple division operation" do
      let input = "=D4/2"
      let expected = BinOp Div (CellRef (CellIndex 4 "D")) (Literal (NumLit 2.0))
      runParser input formulaParser `shouldEqual` Right expected

    it "parses a function with no arguments" do
      let input = "=SUM()"
      let expected = Function (Ident "SUM") []
      runParser input formulaParser `shouldEqual` Right expected

    it "parses a function with a single argument" do
      let input = "=ABS(A1)"
      let expected = Function (Ident "ABS") [ CellRef (CellIndex 1 "A") ]
      runParser input formulaParser `shouldEqual` Right expected

    it "parses a function with multiple arguments" do
      let input = "=SUM(A1,B2,123)"
      let
        expected = Function (Ident "SUM")
          [ CellRef (CellIndex 1 "A")
          , CellRef (CellIndex 2 "B")
          , Literal (NumLit 123.0)
          ]
      runParser input formulaParser `shouldEqual` Right expected

    it "parses nested functions" do
      let input = "=MAX(MIN(A1,B2),C3)"
      let
        expected = Function (Ident "MAX")
          [ Function (Ident "MIN")
              [ CellRef (CellIndex 1 "A")
              , CellRef (CellIndex 2 "B")
              ]
          , CellRef (CellIndex 3 "C")
          ]
      runParser input formulaParser `shouldEqual` Right expected

    it "parses a complex formula with mixed operations" do
      let input = "=A1+B2*C3/D4"
      let
        expected = BinOp Add
          (CellRef (CellIndex 1 "A"))
          ( BinOp Mul
              (CellRef (CellIndex 2 "B"))
              ( BinOp Div
                  (CellRef (CellIndex 3 "C"))
                  (CellRef (CellIndex 4 "D"))
              )
          )
      runParser input formulaParser `shouldEqual` Right expected

    it "parses a function with an operation as argument" do
      let input = "=SUM(A1+B2,C3)"
      let
        expected = Function (Ident "SUM")
          [ BinOp Add (CellRef (CellIndex 1 "A")) (CellRef (CellIndex 2 "B"))
          , CellRef (CellIndex 3 "C")
          ]
      runParser input formulaParser `shouldEqual` Right expected

    it "fails when equals sign is missing" do
      let input = "A1+B2"
      case runParser input formulaParser of
        Left _ -> pure unit
        Right _ -> fail "Parser should have failed but didn't"
