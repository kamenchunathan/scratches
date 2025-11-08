module App where

import Prelude

import Data.Int (decimal, toStringAs)
import Data.Traversable (sequence)
import Data.Maybe as Maybe
import Data.Maybe (Maybe(..), fromMaybe)
import Effect.Class (class MonadEffect)
import Halogen as H
import Halogen.HTML (ClassName(..))
import Halogen.HTML as HH
import Halogen.HTML.Events as HE
import Halogen.HTML.Properties as HP
import Data.String (Pattern(..), toLower)
import Data.String as String 
import Data.Array as Array
import Web.Event.Event (Event)
import Web.Event.Event as WebEvent
import Web.HTML.HTMLInputElement as HTMLInputElement
import Web.UIEvent.KeyboardEvent as KE
import Web.UIEvent.KeyboardEvent (KeyboardEvent)
import Parser as Parser

vocabulary :: Array String
vocabulary = [ "nationality", "current", "team", "age", "position", "goals", "assists" ]

type State = 
  { inputText :: String
  , completions :: Array Parser.Completion
  , selectedIndex :: Int
  , parser :: Parser.ParserConfig
  }

initialState :: forall input. input -> State
initialState _ = 
  { inputText: ""
  , completions: []
  , selectedIndex: 0
  , parser: Parser.createParser vocabulary
  }

data Action 
  = NoOp
  | UpdateInput Event
  | SelectCompletion String
  | HandleKeyDown KeyboardEvent
  | HoverCompletion Int

handleAction :: forall o m. MonadEffect m => Action -> H.HalogenM State Action () o m Unit
handleAction = case _ of
  NoOp -> 
    pure unit
  
  UpdateInput ev -> do
    inp <- WebEvent.target ev >>= HTMLInputElement.fromEventTarget 
      <#> HTMLInputElement.value
      # sequence 
      <#> fromMaybe ""
      # H.liftEffect 
      
    state <- H.get
    let 
      completions = Parser.complete state.parser inp
    
    H.modify_ \st -> st 
      { inputText = inp
      , completions = completions
      , selectedIndex = 0
      }
  
  HandleKeyDown ev -> do
    state <- H.get
    let key = KE.key ev
        completionCount = Array.length state.completions
    
    case key of
      "Tab" -> do
        H.liftEffect $ WebEvent.preventDefault (KE.toEvent ev)
        when (completionCount > 0) do
          let newIndex = (state.selectedIndex + 1) `mod` completionCount
          H.modify_ _ { selectedIndex = newIndex }
      
      "Enter" -> do
        when (completionCount > 0) do
          H.liftEffect $ WebEvent.preventDefault (KE.toEvent ev)
          case Array.index state.completions state.selectedIndex of
            Just completion -> handleAction (SelectCompletion completion.text)
            Nothing -> pure unit
      
      "ArrowDown" -> do
        H.liftEffect $ WebEvent.preventDefault (KE.toEvent ev)
        when (completionCount > 0) do
          let newIndex = (state.selectedIndex + 1) `mod` completionCount
          H.modify_ _ { selectedIndex = newIndex }
      
      "ArrowUp" -> do
        H.liftEffect $ WebEvent.preventDefault (KE.toEvent ev)
        when (completionCount > 0) do
          let newIndex = if state.selectedIndex == 0 
                         then completionCount - 1 
                         else state.selectedIndex - 1
          H.modify_ _ { selectedIndex = newIndex }
      
      _ -> pure unit
    
  SelectCompletion selectedText -> do
    state <- H.get
    let currentToken = Parser.getCurrentToken state.parser state.inputText
        prefix = String.stripSuffix (Pattern currentToken) state.inputText
                  # fromMaybe state.inputText
        newText = prefix <> selectedText <> " "
    
    H.modify_ _ { inputText = newText, completions = [], selectedIndex = 0 }
  
  HoverCompletion idx ->
    H.modify_ _ { selectedIndex = idx }

questionInput :: forall cs m. State -> H.ComponentHTML Action cs m
questionInput state =  
    HH.div
    [ HP.class_ $ ClassName "w-3/4 relative" ]
    [ HH.input 
        [ HP.autofocus true
        , HP.class_ $ ClassName "w-full py-2 px-6 bg-white border border-gray-300 rounded-md shadow-sm focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent"
        , HP.value state.inputText
        , HE.onInput UpdateInput
        , HE.onKeyDown HandleKeyDown
        ] 
    , if Array.null state.completions then
        HH.text ""
      else
        HH.div 
          [ HP.class_ $ ClassName "absolute w-full mt-1 bg-white border border-gray-300 rounded-md shadow-lg max-h-60 overflow-y-auto z-10" ] 
          ( Array.mapWithIndex (renderCompletion state.selectedIndex) state.completions )
    ]

renderCompletion :: forall cs m. Int -> Int -> Parser.Completion -> H.ComponentHTML Action cs m
renderCompletion selectedIndex idx completion =
  let 
    isSelected = idx == selectedIndex
    baseClasses = "px-4 py-2 cursor-pointer transition-colors duration-150"
    stateClasses = if isSelected 
                   then "bg-blue-500 text-white" 
                   else "hover:bg-gray-100 text-gray-900"
    allClasses = baseClasses <> " " <> stateClasses
  in
    HH.div 
      [ HP.class_ $ ClassName allClasses
      , HE.onClick $ const (SelectCompletion completion.text)
      , HE.onMouseEnter $ const (HoverCompletion idx)
      ] 
      [ HH.div [ HP.class_ $ ClassName "font-medium" ] 
          [ HH.text completion.text ]
      , case completion.description of
          Just desc -> 
            HH.div [ HP.class_ $ ClassName "text-sm opacity-75" ] 
              [ HH.text desc ]
          Nothing -> HH.text ""
      ]

render :: forall cs m. State -> H.ComponentHTML Action cs m
render state =
  HH.div
    [ HP.class_ $ ClassName "w-5/6 min-h-screen mx-auto flex flex-col bg-slate-100 items-center" ]
    [ HH.h1 
        [ HP.class_ $ ClassName "text-center text-3xl font-semibold px-4 py-8 text-blue-800" ]
        [ HH.text "Guess the player" ]
    , HH.h2 
        [ HP.class_ $ ClassName "text-center text-lg px-4 py-8 text-gray-700" ] 
        [ HH.text "Type your question into the search bar and guess who the football player is based on the answers" ]
    , HH.div 
        [ HP.class_ $ ClassName "text-center text-sm text-gray-500 mb-4" ]
        [ HH.text "Use Tab or Arrow keys to navigate, Enter to select" ]
    , questionInput state
    ]

component :: forall q o m. MonadEffect m => H.Component q Unit o m
component = H.mkComponent
  { initialState
  , render
  , eval: H.mkEval H.defaultEval { handleAction = handleAction }
  }
