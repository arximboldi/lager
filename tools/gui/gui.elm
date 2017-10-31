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

type alias Step =
    { action: String
    , model: String
    }

type Detail
    = LoadedStep Int Step
    | LoadingStep Int
    | NoStep

type alias Model =
    { server: String
    , status: Status
    , detail: Detail
    }

initStatus = Status "" 0 0
initModel server = Model server initStatus NoStep

init : Flags -> (Model, Cmd Msg)
init flags = let model = initModel flags.server
                 cmd   = queryStatus flags.server
             in (model, cmd)

detailIndex : Detail -> Int
detailIndex d =
    case d of
        LoadedStep idx _ -> idx
        LoadingStep idx  -> idx
        NoStep           -> -1

decodeStatus : Decode.Decoder Status
decodeStatus = Decode.map3 Status
               (Decode.field "program" Decode.string)
               (Decode.field "size"    Decode.int)
               (Decode.field "cursor"  Decode.int)

decodeStep : Decode.Decoder Step
decodeStep = Decode.map2 Step
             (Decode.field "action" Decode.string)
             (Decode.field "model"  Decode.string)

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

viewHistorySelector : Int -> Int -> Html Msg
viewHistorySelector selected idx =
    div [class "step"] [div [] [text (toString idx)]]

viewHeader : Model -> Html Msg
viewHeader model =
    div [ class "header" ]
        [ div [ class "left-side" ]
              [ span [] [text "debugging "]
              , span [class "tt hl"] [text model.status.program]
              , span [] [text " via "]
              , span [class "tt hl"] [text model.server]
              ]
        , div [ class "right-side" ]
            [ span [] [text "program has run "]
            , span [class "hl"] [text (toString model.status.size)]
            , span [class "hl"] [text " steps"]
            ]
        ]

viewDetail : Model -> Html Msg
viewDetail model =
    let content =
            case model.detail of
                LoadedStep idx _ -> [text ("LOADED: " ++ toString idx)]
                LoadingStep idx  -> [text ("LOADING: " ++ toString idx)]
                NoStep           -> [text "NONE"]
    in
        div [ class "detail" ]
            [ div [class "content"] content ]

viewHistory : Model -> Html Msg
viewHistory model =
    let index = detailIndex model.detail
        selectors = List.range 0 model.status.size
                  |> List.map (viewHistorySelector index)
    in
        div [ class "history" ] selectors

view : Model -> Html Msg
view model =
    body []
        [ viewHeader model
        , div [ class "main" ]
            [ viewDetail model
            , viewHistory model]
        ]

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
