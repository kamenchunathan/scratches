module App where

import Prelude

import Data.Int (decimal, toStringAs)
import Data.Traversable (sequence)
import Data.Maybe as Maybe
import Effect.Class (class MonadEffect)
import Halogen as H
import Halogen.HTML (ClassName(..))
import Halogen.HTML as HH
import Halogen.HTML.Events as HE
import Halogen.HTML.Properties as HP
import Data.String (Pattern(..), contains, toLower)
import Data.Array as Array
import Web.Event.Event (Event)
import Web.Event.Event as WebEvent
import Web.HTML.HTMLInputElement as HTMLInputElement

completeions = [ "nationality", "current", "team", "age" ]

type State = { 
  inputText :: String
}

initialState :: forall input. input -> State
initialState _ = { inputText: "" }

data Action 
  = NoOp
  | UpdateInput Event
  | SelectCompletion String

handleAction :: forall o m. MonadEffect m => Action -> H.HalogenM State Action () o m Unit
handleAction = case _ of
  NoOp -> 
    pure unit
  
  UpdateInput ev -> do
    inp <- WebEvent.target ev >>= HTMLInputElement.fromEventTarget 
      <#> HTMLInputElement.value
      # sequence 
      <#> Maybe.fromMaybe ""
      # H.liftEffect 
    H.modify_ \st -> st { inputText = inp }
    
  SelectCompletion selectedText -> 
    H.modify_ \st -> st { inputText = selectedText }

questionInput :: forall cs m. String -> H.ComponentHTML Action cs m
questionInput inp =  
  let
    -- Filter completions based on input text
    lowerInp = toLower inp
    filteredCompletions = 
      if lowerInp == "" then
        []
      else
        Array.filter (contains (Pattern lowerInp) <<< toLower) completeions
  in
  HH.div
    [ HP.class_ $ ClassName "w-3/4 "  ]
    [ HH.input [ 
        HP.autofocus true,
        HP.class_ $ ClassName "w-full py-2 px-6 bg border border-black rounded-md",
        HP.value inp,
        HE.onInput UpdateInput
      ] 
    , HH.div [
          -- TODO: Only add styling when completions has elements
          HP.class_ $ ClassName "w-full py-2 px-6 bg border border-black rounded-md "
        ] 
        ( 
          filteredCompletions <#>
          (\c -> HH.div [ HE.onClick $ const (SelectCompletion c) ] [ HH.text c ])
        )
    ]

render :: forall cs m. State -> H.ComponentHTML Action cs m
render { inputText } =
  HH.div
    [ HP.class_ $ ClassName "w-5/6 min-h-screen mx-auto flex flex-col bg-slate-100 items-center" ]
    [ HH.h1 [ HP.class_ $ ClassName "text-center text-3xl font-semibold px-4 py-8 text-blue-800" ]
          [ HH.text "Guess the player" ]
      , HH.h2 [ HP.class_ $ ClassName "text-center text-lg px-4 py-8" ] 
          [ HH.text "Type your question into the search bar and guess who the football player is based on the answers" ]
      , questionInput inputText
    ]

component :: forall q o m. MonadEffect m => H.Component q Unit o m
component = H.mkComponent
  { initialState
  , render
  , eval: H.mkEval H.defaultEval { handleAction = handleAction }
  }
