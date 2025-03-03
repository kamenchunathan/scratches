module Sheet.Parser where

import Prelude hiding (between)

import Control.Alternative ((<|>))
import Control.Lazy (defer)
import Data.Array (cons)
import Data.Array.NonEmpty as NonEmptyArray
import Data.Cell (CellIndex(..), Expr(..), Ident(..), Literal(..), Operator(..))
import Data.Int as Int
import Data.Maybe (Maybe(..), fromMaybe)
import Data.String.CodeUnits (fromCharArray)
import Data.Tuple (snd)
import Parsing (Parser, fail)
import Parsing.Combinators (between, optionMaybe, try)
import Parsing.Combinators.Array (many, many1, manyIndex)
import Parsing.String (char)
import Parsing.String.Basic (alphaNum, digit, number, upper)

equalSign :: Parser String Char
equalSign = char '='

formulaParser :: Parser String Expr
formulaParser =
  equalSign *> expressionParser

expressionParser :: Parser String Expr
expressionParser = do
  (try $ defer \_ -> functionParser)
    <|> try (defer \_ -> binOpParser)
    <|> try cellRefParser
    <|> try literalParser

operatorParser :: Parser String Operator
operatorParser = do
  char '+' $> Add
    <|> (char '-' $> Sub)
    <|> (char '/' $> Div)
    <|> (char '*' $> Mul)

binOpParser :: Parser String Expr
binOpParser = do
  left <- expressionParser'
  op <- operatorParser
  right <- expressionParser
  pure $ BinOp op left right
  where
  expressionParser' = do
    cellRefParser
      <|> literalParser
      <|> (defer \_ -> functionParser)

cellIndex :: Parser String CellIndex
cellIndex = do
  letterIx <- manyIndex 1 2 (const upper) <#> (snd >>> fromCharArray)
  numIx <- posInt
  pure (CellIndex numIx letterIx)

posInt :: Parser String Int
posInt = do
  numStr <- many digit <#> fromCharArray
  case Int.fromString numStr of
    Just i -> pure i
    Nothing -> fail "Not an int"

ident :: Parser String Ident
ident =
  many1 upper <#> NonEmptyArray.toArray >>> fromCharArray <#> Ident

-- functionParser :: forall m. ParserT String Expr
functionParser :: Parser String Expr
functionParser = do
  name <- ident
  args <- between (char '(') (char ')') argListParser
  pure $ Function name args

argListParser :: Parser String (Array Expr)
argListParser = do
  first <- optionMaybe $ defer \_ -> expressionParser
  rest <- many $ char ',' >>= const expressionParser
  pure $ fromMaybe [] $ cons <$> first <*> Just rest

literalParser :: Parser String Expr
literalParser = numLit <|> stringLit
  where
  numLit =
    number <#> NumLit >>> Literal

  stringLit =
    many1 alphaNum
      <#> NonEmptyArray.toArray >>> fromCharArray
      <#> StrLit >>> Literal

cellRefParser :: Parser String Expr
cellRefParser = cellIndex <#> CellRef
