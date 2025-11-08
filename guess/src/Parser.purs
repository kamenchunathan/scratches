module Parser where

import Prelude

import Data.Array as Array
import Data.Maybe (Maybe(..))
import Data.String (Pattern(..), toLower)
import Data.String as String

type Token = String

type CompletionContext = 
  { previousTokens :: Array Token
  , currentToken :: Token
  , cursorPosition :: Int
  }

type Completion = 
  { text :: String
  , description :: Maybe String
  , priority :: Int  
  }

type ParserConfig =
  { vocabulary :: Array String
  , tokenSeparator :: String
  , caseSensitive :: Boolean
  }

defaultParserConfig :: ParserConfig
defaultParserConfig =
  { vocabulary: []
  , tokenSeparator: " "
  , caseSensitive: false
  }

createParser :: Array String -> ParserConfig
createParser vocab = defaultParserConfig { vocabulary = vocab }

tokenize :: ParserConfig -> String -> Array Token
tokenize config input = 
  String.split (Pattern config.tokenSeparator) input

getCurrentToken :: ParserConfig -> String -> Token
getCurrentToken config input =
  let tokens = tokenize config input
      nonEmpty = Array.filter (\t -> t /= "") tokens
  in Array.last nonEmpty # case _ of
    Just token -> token
    Nothing -> ""

buildContext :: ParserConfig -> String -> CompletionContext
buildContext config input =
  let 
    tokens = tokenize config input
    allTokens = Array.filter (\t -> t /= "") tokens
    current = getCurrentToken config input
    previous = Array.takeWhile (\t -> t /= current) allTokens
  in
    { previousTokens: previous
    , currentToken: current
    , cursorPosition: String.length input
    }

filterCompletions :: ParserConfig -> Token -> Array String
filterCompletions config token =
  if token == "" then
    []
  else
    let 
      compareToken = if config.caseSensitive then token else toLower token
      matchesPrefix word =
        let compareWord = if config.caseSensitive then word else toLower word
        in String.stripPrefix (Pattern compareToken) compareWord
          # case _ of
              Just _ -> true
              Nothing -> false
    in
      Array.filter matchesPrefix config.vocabulary

getCompletions :: ParserConfig -> CompletionContext -> Array Completion
getCompletions config context =
  let 
    matches = filterCompletions config context.currentToken
    toCompletion text = 
      { text: text
      , description: Nothing
      , priority: 0
      }
  in
    map toCompletion matches

-- | Main API: Get completions for input text
complete :: ParserConfig -> String -> Array Completion
complete config input =
  let context = buildContext config input
  in getCompletions config context

completionTexts :: Array Completion -> Array String
completionTexts = map _.text
