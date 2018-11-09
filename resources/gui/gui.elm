--
-- lager - library for functional interactive c++ programs
-- Copyright (C) 2017 Juan Pedro Bolivar Puente
--
-- This file is part of lager.
--
-- lager is free software: you can redistribute it and/or modify
-- it under the terms of the MIT License, as detailed in the LICENSE
-- file located at the root of this source code distribution,
-- or here: <https://github.com/arximboldi/lager/blob/master/LICENSE>
--

port module Main exposing (..)

import Html exposing (..)
import Html.Keyed as Keyed
import Html.Attributes exposing (..)
import Html.Events exposing (..)
import Http
import Json.Decode as Decode
import Json.Encode as Encode
import Time exposing (Time)
import Keyboard.Combo as Keys

type alias Flags = {
    server : String
    }

main = Html.programWithFlags
       { init = init
       , view = view
       , update = update
       , subscriptions = subscriptions
       }

port title : String -> Cmd a

--
-- data model
--

type alias Status =
    { program: String
    , summary: Int
    , cursor: Int
    , paused: Bool
    }

type alias Step =
    { action: Maybe Decode.Value
    , model: Decode.Value
    }

type Detail
    = LoadedStep Int Step
    | ChangingStep Int Step
    | LoadingStep Int
    | NoStep

type alias Model =
    { server: String
    , status: Status
    , detail: Detail
    , keys: Keys.Model Msg
    }

initStatus = Status "" 0 0 False
initModel server = Model server initStatus NoStep (Keys.init keys ComboMsg)

init : Flags -> (Model, Cmd Msg)
init flags = let model = initModel flags.server
                 cmd   = queryStatus flags.server
             in (model, cmd)

detailIndex : Detail -> Int
detailIndex d =
    case d of
        LoadedStep idx _    -> idx
        LoadingStep idx     -> idx
        ChangingStep idx _  -> idx
        NoStep              -> -1

decodeStatus : Decode.Decoder Status
decodeStatus = Decode.map4 Status
               (Decode.field "program" Decode.string)
               (Decode.field "summary"    Decode.int)
               (Decode.field "cursor"  Decode.int)
               (Decode.field "paused"  Decode.bool)

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
         | Pause
         | Resume
         | TogglePause
         | Undo
         | Redo
         | KeyUp
         | KeyDown
         | KeyGoUp
         | KeyGoDown
         | Tick Time
         | ComboMsg Keys.Msg

selectStep : Model -> Int -> (Model, Cmd Msg)
selectStep model index =
    if index < 0 || index > model.status.summary
    then (model, Cmd.none)
    else let detail = case model.detail of
                          LoadedStep _ step   -> ChangingStep index step
                          ChangingStep _ step -> ChangingStep index step
                          LoadingStep _       -> LoadingStep index
                          NoStep              -> LoadingStep index
         in ({ model | detail = detail }, queryStep model.server index)

selectGotoStep : Model -> Int -> (Model, Cmd Msg)
selectGotoStep model index =
    let (newModel, cmd) = selectStep model index
        newCmd = if index == detailIndex newModel.detail
                 then Cmd.batch [cmd, queryGoto model.server index]
                 else cmd
    in (newModel, newCmd)

update : Msg -> Model -> (Model, Cmd Msg)
update msg model =
    case msg of
        RecvStatus (Ok status) ->
            let index = detailIndex model.detail
                newModel = { model | status = status }
            in
                if index == (-1) || (status.cursor /= model.status.cursor &&
                                       index == model.status.cursor)
                then selectStep newModel status.cursor
                else (newModel, title <| "debugging: " ++ status.program)
        RecvStatus (Err err) ->
            Debug.log ("RecvStatus Err: " ++ toString err)
                (model, Cmd.none)
        RecvStep (Ok detail) ->
            if detailIndex detail == detailIndex model.detail
            then ({model | detail = detail}, Cmd.none)
            else (model, Cmd.none)
        RecvStep (Err err) ->
            Debug.log ("RecvStep Err: " ++ toString err)
                (model, Cmd.none)
        RecvPost (Ok _) ->
            (model, Cmd.none)
        RecvPost (Err err) ->
            (model, Cmd.none)
        SelectStep index ->
            selectStep model index
        GotoStep index ->
            (model, queryGoto model.server index)
        Pause ->
            (model, queryPause model.server)
        Resume ->
            (model, queryResume model.server)
        TogglePause ->
            (model, if model.status.paused
                    then queryResume model.server
                    else queryPause model.server)
        Undo ->
            (model, queryUndo model.server)
        Redo ->
            (model, queryRedo model.server)
        KeyUp ->
            selectStep model (detailIndex model.detail - 1)
        KeyDown ->
            selectStep model (detailIndex model.detail + 1)
        KeyGoUp ->
            selectGotoStep model (detailIndex model.detail - 1)
        KeyGoDown ->
            selectGotoStep model (detailIndex model.detail + 1)
        ComboMsg msg ->
            let (keys, cmd) = Keys.update msg model.keys
            in ({ model | keys = keys }, cmd)
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
              [ div [class "block tt hl"] [text model.status.program]
              , div [class "block"] [text model.server]
              , div [class "block"]
                  [ span [class "hl"] [text <| (toString model.status.summary)]
                  , text " steps" ] -- ⸽
              ]
        , div [ class "right-side" ]
            [ viewPlayButton model.status.paused
            , viewRedoButton model.status
            , viewUndoButton model.status
            ]
        ]

viewPlayButton : Bool -> Html Msg
viewPlayButton paused = if paused
                        then div [class "button", onClick Resume] [text "⏵"]
                        else div [class "button", onClick Pause] [text "⏸"]

viewUndoButton : Status -> Html Msg
viewUndoButton status =
    let disabled = status.cursor == 0
    in div [ classes [(True, "button"), (disabled, "disabled")]
           , onClick Undo ]
        [text "⮌"] -- alt: ⮌↶↺⮢⮠⮪⏪⏮

viewRedoButton : Status -> Html Msg
viewRedoButton status =
    let disabled = status.cursor == status.summary
    in div [ classes [(True, "button"), (disabled, "disabled")]
           , onClick Redo ]
        [text "⮎"] -- alt: ⮎↷↻⮣⮡⮫⏩⏭

viewNoStep  = div [class "info"] [text "No step selected"]
viewLoading = div [class "info"] [text "Loading..."]
viewStep step =
    let encode = Encode.encode 4
    in div [] <|
        case step.action of
            Just action ->
                [ div [class "info"] [text "action"]
                , pre [class "code"] [text <| encode action]
                , div [class "info"] [text "model"]
                , pre [class "code"] [text <| encode step.model]]
            Nothing ->
                [ div [class "info"] [text "initial model" ]
                , pre [class "code"] [text <| encode step.model]]

viewDetail : Model -> Html Msg
viewDetail model =
    div [ class "detail" ] <|
        case model.detail of
            LoadedStep idx s   -> [viewStep s]
            ChangingStep idx s -> [viewStep s]
            LoadingStep idx    -> [viewLoading]
            NoStep             -> [viewNoStep]

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
        selectors = List.range 0 model.status.summary
                  |> List.map (\idx ->
                                  ( toString idx
                                  , viewHistoryItem
                                        model.status.cursor
                                        selected idx ))
    in
        Keyed.node "div" [ class "history" ] selectors

view : Model -> Html Msg
view model =
    div [ ]
        [ viewHeader model
        , div [ class "main" ]
            [ viewDetail model
            , viewHistory model]
        ]

--
-- subs
--

keys = [ Keys.combo2 (Keys.control, Keys.z) Undo
       , Keys.combo2 (Keys.control, Keys.y) Redo
       , Keys.combo2 (Keys.shift, Keys.up) KeyGoUp
       , Keys.combo2 (Keys.shift, Keys.down) KeyGoDown
       , Keys.combo1 Keys.up KeyUp
       , Keys.combo1 Keys.down KeyDown
       , Keys.combo1 Keys.space TogglePause
       ]

subscriptions : Model -> Sub Msg
subscriptions model =
    Sub.batch
        [ Time.every (200 * Time.millisecond) Tick
        , Keys.subscriptions model.keys
        ]

--
-- server communication
--

queryStatus : String -> Cmd Msg
queryStatus server =
    let url = server ++ "/api"
    in Http.send RecvStatus (Http.get url decodeStatus)

queryStep : String -> Int -> Cmd Msg
queryStep server index =
    let url = server ++ "/api/step/" ++ toString index
    in Http.send RecvStep <|
        Http.get url <|
            Decode.map (LoadedStep index) decodeStep

queryGoto : String -> Int -> Cmd Msg
queryGoto server index =
    let url = server ++ "/api/goto/" ++ toString index
    in Http.send RecvPost (Http.post url Http.emptyBody (Decode.succeed ()))

queryUndo : String -> Cmd Msg
queryUndo server =
    let url = server ++ "/api/undo"
    in Http.send RecvPost (Http.post url Http.emptyBody (Decode.succeed ()))

queryRedo : String -> Cmd Msg
queryRedo server =
    let url = server ++ "/api/redo"
    in Http.send RecvPost (Http.post url Http.emptyBody (Decode.succeed ()))

queryPause : String -> Cmd Msg
queryPause server =
    let url = server ++ "/api/pause"
    in Http.send RecvPost (Http.post url Http.emptyBody (Decode.succeed ()))

queryResume : String -> Cmd Msg
queryResume server =
    let url = server ++ "/api/resume"
    in Http.send RecvPost (Http.post url Http.emptyBody (Decode.succeed ()))
