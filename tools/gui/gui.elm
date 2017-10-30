module Main exposing (..)

import Html exposing (..)
import Html.Attributes exposing (..)
import Html.Events exposing (..)
import Http
import Json.Decode as Decode
import Time exposing (Time, second)

type alias Flags = {
    server : String
    }

main = Html.programWithFlags
       { init = init
       , view = view
       , update = update
       , subscriptions = subscriptions
       }

--
-- data model
--

type alias Status =
    { program: String
    , size: Int
    , cursor: Int
    }

initStatus = Status "" 0 0

decodeStatus : Decode.Decoder Status
decodeStatus = Decode.map3 Status
               (Decode.field "program" Decode.string)
               (Decode.field "size"    Decode.int)
               (Decode.field "cursor"  Decode.int)

type alias Model =
    { server: String
    , status: Status
    }

init : Flags -> (Model, Cmd Msg)
init flags = ( Model flags.server initStatus
              , queryStatus flags.server
              )

--
-- reducer
--

type Msg = RecvStatus (Result Http.Error Status)
         | Tick Time

update : Msg -> Model -> (Model, Cmd Msg)
update msg model =
    case msg of
        RecvStatus (Ok status) ->
            ({model | status = status}, Cmd.none)
        RecvStatus (Err _) ->
            (model, Cmd.none)
        Tick t ->
            (model, queryStatus model.server)

--
-- view
--

view : Model -> Html Msg
view model =
    div []
        [ div [ class "header" ]
              [ div [ class "left-side" ]
                    [ text model.server ]
              , div [ class "right-side" ]
                    [ text model.status.program ] ]
        , div [ class "main" ]
              [ div [ class "history" ]
                    [ text "x" ]
              , div [ class "detail" ]
                    [ text "main area"] ] ]

--
-- subs
--

subscriptions : Model -> Sub Msg
subscriptions model =
    Time.every second Tick

--
-- server communication
--

queryStatus : String -> Cmd Msg
queryStatus server =
    let url = server ++ "/"
    in Http.send RecvStatus (Http.get url decodeStatus)
