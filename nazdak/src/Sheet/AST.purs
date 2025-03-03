module Sheet.AST where

import Prelude

import Data.Eq.Generic (genericEq)
import Data.Generic.Rep (class Generic)
import Sheet.Index (CellIndex)
import Data.Show.Generic (genericShow)


data Expr
  = Literal Literal
  | CellRef CellIndex
  | BinOp Operator Expr Expr
  | Function Ident (Array Expr)

instance Show Expr where
  show (Literal lit) = show lit
  show (Function funcName args) = show funcName <> "(" <> show args <> ")"
  show (CellRef cIx) = show cIx
  show (BinOp op expr1 expr2) = show expr1 <> show op <> show expr2

instance Eq Expr where
  eq (Literal l) (Literal r) = l == r
  eq (Function l1 l2) (Function r1 r2) = l1 == r1 && l2 == r2
  eq (CellRef l1) (CellRef l2) = l1 == l2
  eq (BinOp l1 l2 l3) (BinOp r1 r2 r3) = l1 == r1 && l2 == r2 && l3 == r3
  eq _ _ = false


data Literal
  = NumLit Number
  | StrLit String

derive instance Generic Literal _

instance Show Literal where
  show = genericShow

instance Eq Literal where
  eq = genericEq


newtype Ident = Ident String

derive newtype instance Show Ident
derive newtype instance Eq Ident

data Operator
  = Add
  | Sub
  | Mul
  | Div

derive instance Generic Operator _

instance Eq Operator where
  eq = genericEq

instance Show Operator where
  show = genericShow

