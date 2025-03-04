module Sheet
  ( Sheet(..)
  , empty
  , index
  , sample
  , updateCell
  ) where

import Prelude

import Data.Map (Map)
import Data.Map as Map
import Data.Maybe (Maybe(..), fromJust)
import Data.Newtype (class Newtype)
import Data.Tuple.Nested ((/\))
import Data.Unfoldable (unfoldr)
import Partial.Unsafe (unsafePartial)
import Sheet.AST (Literal(..))
import Sheet.Cell (Cell(..))
import Sheet.Index (CellIndex(..), alphaIxFromNum)

newtype Sheet = Sheet (Map CellIndex Cell)

derive instance Newtype Sheet _
derive newtype instance Show Sheet

index :: Sheet -> CellIndex -> Maybe Cell
index (Sheet inner) i = Map.lookup i inner

-- / A 10 * 10 Sheet
sample :: Sheet
sample =
  let
    f n | n < 0 = Nothing
    f n = Just $
      ( CellIndex (n / 10) (unsafePartial (fromJust $ alphaIxFromNum (n `mod` 10)))
          /\ (Simple (StrLit $ show n))
      )
        /\ (n - 1)

  in
    Sheet (Map.fromFoldable $ unfoldr @Array f 99)

empty :: Int -> Int -> Sheet
empty n m =
  let
    f x | x < 0 = Nothing
    f x = Just $
      ( CellIndex (x / n) (unsafePartial (fromJust $ alphaIxFromNum (x `mod` m)))
          /\ (Simple (StrLit ""))
      )
        /\ (x - 1)

  in
    Sheet (Map.fromFoldable $ unfoldr @Array f (n * m))

updateCell :: CellIndex -> Cell -> Sheet -> Sheet
updateCell idx cell (Sheet cells) = Sheet (Map.insert idx cell cells)
