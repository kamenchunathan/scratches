module Main where

import Prelude hiding ((/))

import Effect.Aff (Aff)
import HTTPurple
  ( class Generic
  , Request
  , Response
  , RouteDuplex'
  , ServerM
  , mkRoute
  , noArgs
  , segment
  , serve
  , (/)
  )
import HTTPurple.Headers (header)
import HTTPurple.Response (ok')
import Node.Encoding (Encoding(..))
import Node.FS.Sync (readTextFile)

data Route
  = Home
  | Login String

derive instance Generic Route _

route :: RouteDuplex' Route
route = mkRoute
  { "Home": noArgs
  , "Login": "auth/login" / segment

  }

type AppData = { homeHtml :: String }

main :: ServerM
main = do
  homeHtml <- readTextFile UTF8 "./templates/index.html"
  serve { hostname: "localhost", port: 5173 }
    { route, router: router { homeHtml } }

router :: AppData -> Request Route -> Aff Response
router dat { route: Home } = serveRoot dat
router dat { route: (Login next) } = serveRoot dat

serveRoot :: AppData -> Aff Response
serveRoot { homeHtml } = ok' (header "Content-Type" "text/html") homeHtml
