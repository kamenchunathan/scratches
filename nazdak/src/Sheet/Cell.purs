module Sheet.Cell where

import Prelude

import Data.Generic.Rep (class Generic)
import Data.Show.Generic (genericShow)
import Sheet.AST (Expr)

data Cell
  -- Treated as text Data types such as number, date, can be inferred and formatting done
  -- Inference of type is useful for evaluation and use of cells in other cells
  = Text String
  -- Preceded by an equal sign and is recomputed when inputs change
  | Formula Expr

derive instance Generic Cell _

instance Show Cell where
  show = genericShow



