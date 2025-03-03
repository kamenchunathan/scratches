module Sheet where

import Prelude

import Data.Cell (CellIndex(..))
import Data.Map (Map)
import Data.Maybe (Maybe(..), fromJust)
import Data.Newtype (class Newtype)
import Data.Tuple.Nested ((/\))
import Partial.Unsafe (unsafePartial)

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
          /\ (Text $ show n)
      )
        /\ (n - 1)

  in
    Sheet (Map.fromFoldable $ unfoldr @Array f 99)
    
empty :: Sheet
empty n m =
  let
    f n | n < 0 = Nothing
    f n = Just $
      ( CellIndex (n / 10) (unsafePartial (fromJust $ alphaIxFromNum (n `mod` 10)))
          /\ (Text "")
      )
        /\ (n - 1)

  in
    Sheet (Map.fromFoldable $ unfoldr @Array f 99)

