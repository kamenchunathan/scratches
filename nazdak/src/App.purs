module App where

import Prelude

import Halogen (ClassName(..))
import Halogen as H
import Halogen.HTML as HH
import Halogen.HTML.Events as HE
import Halogen.HTML.Properties as HP

type State = {}

data Action = NoOp

initialState :: forall input. input -> State
initialState _ = {}

handleAction
  :: forall output m
   . Action
  -> H.HalogenM State Action () output m Unit
handleAction _ = pure unit

render :: forall m. State -> H.ComponentHTML Action () m
render _ = HH.div
  [ HP.class_ (ClassName "mx-auto min-h-screen container py-8") ]
  [ HH.div
      [ HP.class_ (ClassName "mx-auto py-2 flex space-x-4") ]
      [ HH.text "Hello world" ]
  ]

component :: forall q i o m. H.Component q i o m
component = H.mkComponent
  { initialState
  , render
  , eval: H.mkEval (H.defaultEval { handleAction = handleAction })
  }
