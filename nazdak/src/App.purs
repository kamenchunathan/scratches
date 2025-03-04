module App where

import Prelude

import Data.Array ((:))
import Data.Array as Array
import Data.Map as Map
import Data.Maybe (fromJust, fromMaybe)
import Data.Newtype (unwrap)
import Effect.Class (class MonadEffect, liftEffect)
import Effect.Class.Console (log)
import Halogen (AttrName(..), ClassName(..))
import Halogen as H
import Halogen.HTML as HH
import Halogen.HTML.Events as HE
import Halogen.HTML.Properties as HP
import Partial.Unsafe (unsafePartial)
import Sheet (Sheet)
import Sheet as Sheet
import Sheet.Cell (Cell(..))
import Sheet.Index (CellIndex(..), alphaIxFromNum)
import Unsafe.Coerce (unsafeCoerce)
import Web.DOM.Node as Node
import Web.Event.Event (Event)
import Web.Event.Event as Event

type State = { spreadSheet :: Sheet }

data Action =
  CellInput CellIndex Event

initialState :: forall input. input -> State
initialState _ = { spreadSheet: Sheet.sample }

handleAction
  :: forall output m
   . MonadEffect m
  => Action
  -> H.HalogenM State Action () output m Unit
handleAction = case _ of
  CellInput ix ev -> do
    log $ unsafeCoerce ev
    cellContent <- Event.target ev
      >>= Node.fromEventTarget
      <#> Node.textContent
      # fromMaybe (pure "")
      # liftEffect
    log $ show ix <> cellContent

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

renderCol :: forall m. Sheet -> String -> H.ComponentHTML Action () m
renderCol spreadSheet colIndex =
  HH.div
    [ HP.class_ (ClassName "first:border-l border-r border-gray-400") ]
    (
      -- Column name
      ( HH.div
          [ HP.class_
              (ClassName "h-10 px-2 py-2 border-y border-gray-400 bg-gray-200")
          ]
          [ HH.text colIndex ]
      )
        :

          -- Spreadsheet cells
          ( spreadSheet
              # unwrap
              # Map.filterKeys (\(CellIndex _ col) -> col == colIndex)
              # Map.keys
              # Array.fromFoldable
              <#> renderCell spreadSheet
          )
    )

renderCell :: forall m. Sheet -> CellIndex -> H.ComponentHTML Action () m
renderCell sheet ix =
  Sheet.index sheet ix
    <#> renderCellContent
    # fromMaybe (HH.text "Error")
    # renderCellWrapper
  where
  renderCellWrapper innerHtml =
    HH.div
      [ HP.attr (AttrName "contenteditable") "plaintext-only"
      , HP.class_
          (ClassName "h-10 px-2 py-2 border-b border-gray-400")
      -- TODO: Setting the width explicitly allows this to be changed  by user
      --   Add this in future
      , HP.style $ "width: " <> (show 8) <> "rem;"
      , HE.onInput $ CellInput ix
      ]
      [ innerHtml ]

  renderCellContent (Text t) = HH.text t
  renderCellContent (Formula _) = HH.text "Formula"

component :: forall q i o m. MonadEffect m => H.Component q i o m
component = H.mkComponent
  { initialState
  , render
  , eval: H.mkEval (H.defaultEval { handleAction = handleAction })
  }
