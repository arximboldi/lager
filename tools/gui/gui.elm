module Main exposing (..)

import Html exposing (..)
import Html.Keyed as Keyed
import Html.Attributes exposing (..)
import Html.Events exposing (..)
import Http
import Json.Decode as Decode
import Json.Encode as Encode
import Time exposing (Time)

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
    { action: Maybe Decode.Value
    , model: Decode.Value
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
             (Decode.maybe <| Decode.field "action" Decode.value)
             (Decode.field "model"  Decode.value)

--
-- reducer
--

type Msg = RecvStatus (Result Http.Error Status)
         | RecvStep (Result Http.Error Detail)
         | RecvPost (Result Http.Error ())
         | SelectStep Int
         | GotoStep Int
         | Tick Time

update : Msg -> Model -> (Model, Cmd Msg)
update msg model =
    case msg of
        RecvStatus (Ok status) ->
            ({model | status = status}, Cmd.none)
        RecvStatus (Err err) ->
            Debug.log ("RecvStatus Err: " ++ toString err)
                (model, Cmd.none)
        RecvStep (Ok detail) ->
            ({model | detail = detail}, Cmd.none)
        RecvStep (Err err) ->
            Debug.log ("RecvStep Err: " ++ toString err)
                (model, Cmd.none)
        RecvPost (Ok _) ->
            (model, Cmd.none)
        RecvPost (Err err) ->
            (model, Cmd.none)
        SelectStep index ->
            (model, queryStep model.server index)
        GotoStep index ->
            (model, queryGoto model.server index)
        Tick t ->
            (model, queryStatus model.server)

--
-- view
--

classes : List (Bool, String) -> Attribute Msg
classes cls = List.filter Tuple.first cls
            |> List.map Tuple.second
            |> String.join " "
            |> class

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

viewLoading = div [class "info"] [text "Loading..."]
viewNoStep  = div [class "info"] [text "No step selected"]
viewStep step =
    let encode = Encode.encode 4
    in div [] <|
        case step.action of
            Just action ->
                [ div [class "info"] [text "action"]
                , div [class "code"] [text <| encode action]
                , div [class "info"] [text "model"]
                , div [class "code"] [text <| encode step.model]]
            Nothing ->
                [ div [class "info"] [text "initial" ]
                , div [class "code"] [text <| encode step.model]]

viewDetail : Model -> Html Msg
viewDetail model =
    div [ class "detail" ] <|
        case model.detail of
            LoadedStep idx s -> [viewStep s]
            LoadingStep idx  -> [viewLoading]
            NoStep           -> [viewNoStep]

viewHistoryItem : Int -> Int -> Int -> Html Msg
viewHistoryItem cursor selected idx =
    div [ classes [ (True, "step")
                  , (selected == idx, "selected")
                  , (cursor == idx, "cursor")]
        , onClick (SelectStep idx)
        , onDoubleClick (GotoStep idx)
        ]
        [div [] [text (toString idx)]]

viewHistory : Model -> Html Msg
viewHistory model =
    let selected = detailIndex model.detail
        selectors = List.range 0 model.status.size
                  |> List.map (\idx ->
                                  ( toString idx
                                  , viewHistoryItem
                                        model.status.cursor
                                        selected idx ))
    in
        Keyed.node "div" [ class "history" ] selectors

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
    Time.every (200 * Time.millisecond) Tick

--
-- server communication
--

queryStatus : String -> Cmd Msg
queryStatus server =
    let url = server ++ "/"
    in Http.send RecvStatus (Http.get url decodeStatus)

queryStep : String -> Int -> Cmd Msg
queryStep server index =
    let url = server ++ "/step?cursor=" ++ toString index
    in Http.send RecvStep <|
        Http.get url <|
            Decode.map (LoadedStep index) decodeStep

queryGoto : String -> Int -> Cmd Msg
queryGoto server index =
    let url = server ++ "/goto?cursor=" ++ toString index
    in Http.send RecvPost (Http.post url Http.emptyBody (Decode.succeed ()))
