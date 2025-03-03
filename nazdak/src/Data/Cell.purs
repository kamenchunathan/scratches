module Data.Cell
  ( Cell(..)
  , CellIndex(..)
  , Expr(..)
  , Ident(..)
  , Literal(..)
  , Operator(..)
  , Sheet(..)
  , alphaIxFromNum
  , emptySheet
  , index
  ) where

import Prelude

import Data.Eq.Generic (genericEq)
import Data.Generic.Rep (class Generic)
import Data.Map (Map)
import Data.Map as Map
import Data.Maybe (Maybe(..), fromJust)
import Data.Newtype (class Newtype)
import Data.Show.Generic (genericShow)
import Data.String (codePointAt, singleton) as String
import Data.Tuple.Nested ((/\))
import Data.Unfoldable (unfoldr)
import Partial.Unsafe (unsafePartial)

data Cell
  -- Treated as text Data types such as number, date, can be inferred and formatting done
  -- Inference of type is useful for evaluation and use of cells in other cells
  = Text String
  -- Preceded by an equal sign and is recomputed when inputs change
  | Formula Expr

derive instance Generic Cell _

instance Show Cell where
  show = genericShow

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

data CellIndex = CellIndex Int String

derive instance Generic CellIndex _

instance Show CellIndex where
  show = genericShow

instance Eq CellIndex where
  eq (CellIndex i1 j1) (CellIndex i2 j2) = i1 == i2 && j1 == j2

instance Ord CellIndex where
  compare (CellIndex i1 j1) (CellIndex i2 j2) | i1 == i2 = compare j1 j2
  compare (CellIndex i1 _) (CellIndex i2 _) = compare i1 i2

alphabet :: String
alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"

alphaIxFromNum :: Int -> Maybe String
alphaIxFromNum n =
  case (n / 26 /\ n `mod` 26) of
    (0 /\ j) -> (String.codePointAt j alphabet) <#> String.singleton
    (i /\ j) -> (<>) <$> (f (i - 1)) <*> (f j)
      where
      f l = String.singleton <$> String.codePointAt l alphabet

newtype Sheet = Sheet (Map CellIndex Cell)

derive instance Newtype Sheet _
derive newtype instance Show Sheet

index :: Sheet -> CellIndex -> Maybe Cell
index (Sheet inner) i = Map.lookup i inner

-- / A 10 * 10 Sheet
emptySheet :: Sheet
emptySheet =
  let
    f n | n < 0 = Nothing
    f n = Just $
      ( CellIndex (n / 10) (unsafePartial (fromJust $ alphaIxFromNum (n `mod` 10)))
          /\ (Text $ show n)
      )
        /\ (n - 1)

  in
    Sheet (Map.fromFoldable $ unfoldr @Array f 99)

