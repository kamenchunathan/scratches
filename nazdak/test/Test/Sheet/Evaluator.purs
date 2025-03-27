module Test.Sheet.Evaluator where

import Prelude

import Data.Either (Either(..))
import Parsing (runParser)
import Sheet (Sheet, empty, updateCell)
import Sheet.AST (Expr(..), Ident(..), Literal(..), Operator(..))
import Sheet.Cell (Cell(..))
import Sheet.Evaluator (Value(..), evaluateCell, evaluateExpr, parseAndEvaluate)
import Sheet.Index (CellIndex(..))
import Sheet.Parser (formulaParser)
import Test.Spec (Spec, describe, it)
import Test.Spec.Assertions (shouldEqual)

-- | Helper function to create a Formula cell from a string
createFormula :: String -> Cell
createFormula input =
  case runParser input formulaParser of
    Right expr -> Formula expr
    Left _ -> Simple $ StrLit "#ERROR"

-- | Helper function to build a test sheet
buildTestSheet :: Sheet
buildTestSheet =
  empty 5 4
    -- Add numeric values
    # updateCell (CellIndex 1 "A") (Simple $ NumLit 10.0)
    # updateCell (CellIndex 2 "A") (Simple $ NumLit 20.0)
    # updateCell (CellIndex 3 "A") (Simple $ NumLit 30.0)
    -- Add string values
    # updateCell (CellIndex 1 "B") (Simple $ StrLit "Hello")
    # updateCell (CellIndex 2 "B") (Simple $ StrLit "World")
    -- Add formula cells
    # updateCell (CellIndex 1 "C") (createFormula "=A1+A2")
    # updateCell (CellIndex 2 "C") (createFormula "=SUM(A1,A2,A3)")
    # updateCell (CellIndex 3 "C") (createFormula "=A2/A1")
    # updateCell (CellIndex 4 "C") (createFormula "=CONCAT(B1,B2)")

tests :: Spec Unit
tests = describe "Sheet.Evaluator" do
  let testSheet = buildTestSheet

  describe "literal evaluation" do
    it "evaluates numeric literals" do
      evaluateExpr testSheet (Literal (NumLit 42.0)) `shouldEqual` NumberVal 42.0

    it "evaluates string literals" do
      evaluateExpr testSheet (Literal (StrLit "test")) `shouldEqual` StringVal "test"

  describe "cell reference evaluation" do
    it "evaluates text cell references" do
      evaluateExpr testSheet (CellRef (CellIndex 1 "A")) `shouldEqual` NumberVal 10.0
      evaluateExpr testSheet (CellRef (CellIndex 1 "B")) `shouldEqual` StringVal "Hello"

    it "evaluates formula cell references" do
      evaluateExpr testSheet (CellRef (CellIndex 1 "C")) `shouldEqual` NumberVal 30.0

    it "handles non-existent cell references" do
      evaluateExpr testSheet (CellRef (CellIndex 99 "Z")) `shouldEqual` ErrorVal "REF"

  describe "binary operations" do
    it "evaluates addition of numbers" do
      evaluateExpr testSheet (BinOp Add (Literal (NumLit 5.0)) (Literal (NumLit 3.0)))
        `shouldEqual` NumberVal 8.0

    it "evaluates subtraction of numbers" do
      evaluateExpr testSheet (BinOp Sub (Literal (NumLit 5.0)) (Literal (NumLit 3.0)))
        `shouldEqual` NumberVal 2.0

    it "evaluates multiplication of numbers" do
      evaluateExpr testSheet (BinOp Mul (Literal (NumLit 5.0)) (Literal (NumLit 3.0)))
        `shouldEqual` NumberVal 15.0

    it "evaluates division of numbers" do
      evaluateExpr testSheet (BinOp Div (Literal (NumLit 6.0)) (Literal (NumLit 3.0)))
        `shouldEqual` NumberVal 2.0

    it "handles division by zero" do
      evaluateExpr testSheet (BinOp Div (Literal (NumLit 5.0)) (Literal (NumLit 0.0)))
        `shouldEqual` ErrorVal "DIV/0"

    it "propagates errors in binary operations" do
      evaluateExpr testSheet (BinOp Add (Literal (NumLit 5.0)) (CellRef (CellIndex 99 "Z")))
        `shouldEqual` ErrorVal "REF"

    it "reports type error for incompatible operations" do
      evaluateExpr testSheet (BinOp Mul (Literal (StrLit "Hello")) (Literal (NumLit 3.0)))
        `shouldEqual` ErrorVal "TYPE"

  describe "function evaluation" do
    it "evaluates SUM function" do
      evaluateExpr testSheet
        ( Function (Ident "SUM")
            [ Literal (NumLit 10.0)
            , Literal (NumLit 20.0)
            , Literal (NumLit 30.0)
            ]
        ) `shouldEqual` NumberVal 60.0

    it "evaluates AVERAGE function" do
      evaluateExpr testSheet
        ( Function (Ident "AVERAGE")
            [ Literal (NumLit 10.0)
            , Literal (NumLit 20.0)
            , Literal (NumLit 30.0)
            ]
        ) `shouldEqual` NumberVal 20.0

    it "evaluates MIN function" do
      evaluateExpr testSheet
        ( Function (Ident "MIN")
            [ Literal (NumLit 10.0)
            , Literal (NumLit 5.0)
            , Literal (NumLit 30.0)
            ]
        ) `shouldEqual` NumberVal 5.0

    it "evaluates MAX function" do
      evaluateExpr testSheet
        ( Function (Ident "MAX")
            [ Literal (NumLit 10.0)
            , Literal (NumLit 20.0)
            , Literal (NumLit 5.0)
            ]
        ) `shouldEqual` NumberVal 20.0

    it "evaluates COUNT function" do
      evaluateExpr testSheet
        ( Function (Ident "COUNT")
            [ Literal (NumLit 10.0)
            , Literal (StrLit "test")
            , Literal (NumLit 30.0)
            ]
        ) `shouldEqual` NumberVal 3.0

    it "evaluates CONCAT function" do
      evaluateExpr testSheet
        ( Function (Ident "CONCAT")
            [ Literal (StrLit "Hello ")
            , Literal (StrLit "World")
            , Literal (NumLit 123.0)
            ]
        ) `shouldEqual` StringVal "Hello World123.0"

    it "handles unknown functions" do
      evaluateExpr testSheet
        ( Function (Ident "UNKNOWN")
            [ Literal (NumLit 10.0)
            , Literal (NumLit 20.0)
            ]
        ) `shouldEqual` ErrorVal "FUNC"

    it "propagates errors in function arguments" do
      evaluateExpr testSheet
        ( Function (Ident "SUM")
            [ Literal (NumLit 10.0)
            , CellRef (CellIndex 99 "Z")
            , Literal (NumLit 30.0)
            ]
        ) `shouldEqual` ErrorVal "TYPE"

  describe "string formula parsing and evaluation" do
    it "parses and evaluates a simple formula" do
      parseAndEvaluate testSheet "=A1" `shouldEqual` (NumberVal 10.0)

    it "parses and evaluates a function formula" do
      parseAndEvaluate testSheet "=SUM(A1,A2,A3)" `shouldEqual` NumberVal 60.0

    it "handles parse errors" do
      parseAndEvaluate testSheet "A1+A2" `shouldEqual` ErrorVal "PARSE"

  describe "cell evaluation" do
    it "evaluates text cells" do
      evaluateCell testSheet (Simple $ StrLit "Hello") `shouldEqual` StringVal "Hello"
      
    it "evaluates numeric cells" do
      evaluateCell testSheet (createFormula "=A1+A2") `shouldEqual` NumberVal 30.0

    it "evaluates formula cells" do
      evaluateCell testSheet (createFormula "=A1+A2") `shouldEqual` NumberVal 30.0

  describe "complex formulas" do
    it "evaluates nested expressions" do
      parseAndEvaluate testSheet "=SUM(A1,A2*2,A3/2)" `shouldEqual` NumberVal 65.0

    it "evaluates cell references in functions" do
      parseAndEvaluate testSheet "=MIN(A1,A2,A3)" `shouldEqual` NumberVal 10.0
