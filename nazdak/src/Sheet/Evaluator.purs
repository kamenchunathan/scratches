module Sheet.Evaluator where

import Prelude

import Data.Array (foldl, length)
import Data.Cell
  ( Cell(..)
  , CellIndex
  , Expr(..)
  , Ident(..)
  , Literal(..)
  , Operator(..)
  , Sheet(..)
  )
import Data.Cell as Cell
import Data.Either (Either(..), note)
import Data.Map as Map
import Data.Maybe (Maybe(..))
import Data.Int (toNumber) as Int
import Data.Traversable (traverse)
import Parsing (runParser)
import Sheet.Parser (formulaParser)

-- | Result of evaluation
data Value
  = NumberVal Number
  | StringVal String
  | ErrorVal String

derive instance eqValue :: Eq Value

instance showValue :: Show Value where
  show (NumberVal n) = show n
  show (StringVal s) = s
  show (ErrorVal e) = "#" <> e

-- | Evaluate a Cell to get its value
evaluateCell :: Sheet -> Cell -> Value
evaluateCell _ (Text s) = StringVal s
evaluateCell sheet (Formula expr) = evaluateExpr sheet expr

-- | Evaluate an expression given the current sheet state
evaluateExpr :: Sheet -> Expr -> Value
evaluateExpr sheet expr = case expr of
  Literal lit -> evalLiteral lit
  CellRef idx -> evaluateCellRef sheet idx
  BinOp op left right -> evalBinOp op (evaluateExpr sheet left) (evaluateExpr sheet right)
  Function name args -> evalFunction sheet name (map (evaluateExpr sheet) args)

-- | Evaluate a cell reference
evaluateCellRef :: Sheet -> CellIndex -> Value
evaluateCellRef sheet idx =
  case Cell.index sheet idx of
    Just cell -> evaluateCell sheet cell
    Nothing -> ErrorVal "REF"

-- | Evaluate a literal expression
evalLiteral :: Literal -> Value
evalLiteral (NumLit n) = NumberVal n
evalLiteral (StrLit s) = StringVal s

-- | Evaluate a binary operation
evalBinOp :: Operator -> Value -> Value -> Value
evalBinOp op (NumberVal left) (NumberVal right) = case op of
  Add -> NumberVal (left + right)
  Sub -> NumberVal (left - right)
  Mul -> NumberVal (left * right)
  Div ->
    if right == 0.0 then ErrorVal "DIV/0"
    else NumberVal (left / right)
evalBinOp _ (ErrorVal e) _ = ErrorVal e
evalBinOp _ _ (ErrorVal e) = ErrorVal e
evalBinOp Add (StringVal s1) (StringVal s2) = StringVal (s1 <> s2)
evalBinOp _ _ _ = ErrorVal "TYPE"

-- | Convert a value to number for calculation
toNumber :: Value -> Maybe Number
toNumber (NumberVal n) = Just n
toNumber (StringVal _) = Nothing
toNumber (ErrorVal _) = Nothing

-- | Helper function to get numeric values from args
numericArgs :: Array Value -> Either String (Array Number)
numericArgs values = traverse toNumber values # note "TYPE"

-- | Evaluate a function with given arguments
evalFunction :: Sheet -> Ident -> Array Value -> Value
evalFunction _ (Ident "SUM") args =
  case numericArgs args of
    Right nums -> NumberVal $ foldl (+) 0.0 nums
    Left err -> ErrorVal err

evalFunction _ (Ident "AVERAGE") args =
  case numericArgs args of
    Right nums ->
      if length nums == 0 then ErrorVal "DIV/0"
      else NumberVal $ (foldl (+) 0.0 nums) / Int.toNumber (length nums)
    Left err -> ErrorVal err

evalFunction _ (Ident "MIN") args =
  case numericArgs args of
    Right [] -> ErrorVal "EMPTY"
    Right nums -> NumberVal $ foldl min (1.0 / 0.0) nums
    Left err -> ErrorVal err

evalFunction _ (Ident "MAX") args =
  case numericArgs args of
    Right [] -> ErrorVal "EMPTY"
    Right nums -> NumberVal $ foldl max (-1.0 / 0.0) nums
    Left err -> ErrorVal err

evalFunction _ (Ident "COUNT") args =
  NumberVal $ Int.toNumber $ length args

evalFunction _ (Ident "IF") args =
  case args of
    [ condition, trueVal, falseVal ] ->
      case condition of
        NumberVal n -> if n /= 0.0 then trueVal else falseVal
        StringVal "" -> falseVal
        StringVal _ -> trueVal
        ErrorVal _ -> condition
    _ -> ErrorVal "ARGS"

evalFunction _ (Ident "CONCAT") args =
  StringVal $ foldl (\acc val -> acc <> valueToString val) "" args
  where
  valueToString (StringVal s) = s
  valueToString (NumberVal n) = show n
  valueToString (ErrorVal e) = "#" <> e

evalFunction _ (Ident _) _ = ErrorVal "FUNC"

-- | Parse a string as a formula and evaluate it
parseAndEvaluate :: Sheet -> String -> Value
parseAndEvaluate sheet input =
  case runParser input formulaParser of
    Right expr -> evaluateExpr sheet expr
    Left _ -> ErrorVal "PARSE"

-- | Reevaluate the entire sheet (returns a new sheet with updated values)
reevaluateSheet :: Sheet -> Sheet
reevaluateSheet (Sheet cells) =
  let
    evaluatedCells = Map.mapMaybeWithKey evaluateCell' cells
  in
    Sheet evaluatedCells
  where
  -- TODO: refactor cell type to have a raw value and computed value
  evaluateCell' _ cell = Just cell

-- | Helper function to update a cell in the sheet
updateCell :: CellIndex -> Cell -> Sheet -> Sheet
updateCell idx cell (Sheet cells) = Sheet (Map.insert idx cell cells)

-- | Helper function to parse a formula string and create a Formula cell
parseFormula :: String -> Either String Cell
parseFormula input =
  case runParser input formulaParser of
    Right expr -> Right (Formula expr)
    Left _ -> Left "Parse error"
