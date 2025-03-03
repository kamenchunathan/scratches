module App where

import Prelude

import Data.Array as Array
import Data.Cell (alphaIxFromNum)
import Data.Cell as Cell
import Data.Map as Map
import Data.Maybe (fromJust)
import Data.Newtype (unwrap)
import Effect.Class (class MonadEffect, liftEffect)
import Effect.Console (log)
import Halogen (AttrName(..), ClassName(..))
import Halogen as H
import Halogen.HTML as HH
import Halogen.HTML.Events as HE
import Halogen.HTML.Properties as HP
import Partial.Unsafe (unsafePartial)
import Unsafe.Coerce (unsafeCoerce)
import Web.Event.Event (Event)

type State = { spreadSheet :: Cell.Sheet }

data Action =
  CellInput Event

initialState :: forall input. input -> State
initialState _ = { spreadSheet: Cell.emptySheet }

handleAction
  :: forall output m
   . MonadEffect m
  => Action
  -> H.HalogenM State Action () output m Unit
handleAction = case _ of
  CellInput ev -> do
    liftEffect $ log $ unsafeCoerce ev

render :: forall m. State -> H.ComponentHTML Action () m
render { spreadSheet } = HH.div
  [ HP.class_ (ClassName "mx-auto min-h-screen container py-8") ]
  [ HH.div []
      [ HH.div
          [ HP.class_ (ClassName "overflow-auto") ]
          [ HH.div
              [ HP.class_ (ClassName "flex") ]
              ( Array.range 0 9
                  <#> (\i -> unsafePartial (fromJust $ alphaIxFromNum i))
                  <#> renderCol spreadSheet
              )
          ]
      ]
  ]

renderCol :: forall m. Cell.Sheet -> String -> H.ComponentHTML Action () m
renderCol spreadSheet colIndex =
  HH.div
    [ HP.class_ (ClassName "first:border-l border-r border-gray-400") ]
    ( spreadSheet
        # unwrap
        # Map.filterKeys (\(Cell.CellIndex _ col) -> col == colIndex)
        # Map.values
        # Array.fromFoldable
        <#> renderCell
    )

renderCell ∷ ∀ m. Cell.Cell → H.ComponentHTML Action () m
renderCell (Cell.Formula _) = HH.div [] [ HH.text "Formula" ]
renderCell (Cell.Text t) =
  HH.div
    [ HP.attr (AttrName "contenteditable") "plaintext-only"
    , HP.class_ (ClassName "px-2 py-1 first:border-t border-b border-gray-400")
    , HP.style $ "width: " <> (show 8) <> "rem;"
    , HE.onInput CellInput
    ]
    [ HH.text t ]

component :: forall q i o m. MonadEffect m => H.Component q i o m
component = H.mkComponent
  { initialState
  , render
  , eval: H.mkEval (H.defaultEval { handleAction = handleAction })
  }
