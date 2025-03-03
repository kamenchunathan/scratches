module Sheet.Index where

import Prelude

import Data.Generic.Rep (class Generic)
import Data.Maybe (Maybe)
import Data.Show.Generic (genericShow)
import Data.String as String
import Data.Tuple.Nested ((/\))


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


