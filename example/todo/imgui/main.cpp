//
// lager - library for functional interactive c++ programs
// Copyright (C) 2017 Juan Pedro Bolivar Puente
//
// This file is part of lager.
//
// lager is free software: you can redistribute it and/or modify
// it under the terms of the MIT License, as detailed in the LICENSE
// file located at the root of this source code distribution,
// or here: <https://github.com/arximboldi/lager/blob/master/LICENSE>
//

#include "../model.hpp"

#include <lager/event_loop/sdl.hpp>
#include <lager/store.hpp>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl.h>

#include <SDL.h>
#include <SDL_opengl.h>

#include <iostream>

constexpr int window_padding = 48;
constexpr int window_width   = 800;
constexpr int window_height  = 600;

// Sadly, ImGui sometimes forces us to store transient state, like text inputs.
// We use this store this.
struct ui_state
{
    static constexpr std::size_t input_string_size = 1 << 10;

    std::array<char, input_string_size> new_todo_input{'\0'};
};

void draw(lager::context<todo::item_action> ctx, const todo::item& i)
{
    auto checked = i.done;
    if (ImGui::Checkbox("", &checked)) {
        ctx.dispatch(todo::toggle_item_action{});
    }

    ImGui::SameLine();
    ImGui::Text("%s", i.text.c_str());

    ImGui::SameLine();
    if (ImGui::Button("Delete")) {
        ctx.dispatch(todo::remove_item_action{});
    }
}

void draw(lager::context<todo::model_action> ctx,
          const todo::model& m,
          ui_state& s)
{
    ImGui::SetNextWindowPos({window_padding, window_padding}, ImGuiCond_Once);
    ImGui::SetNextWindowSize(
        {window_width - 2 * window_padding, window_height - 2 * window_padding},
        ImGuiCond_Once);
    ImGui::Begin("Todo app");

    if (ImGui::BeginPopup("not-implemented")) {
        ImGui::Text("Saving and loading have not been implemented!");
        ImGui::EndPopup();
    }

    if (ImGui::Button("Save"))
        ImGui::OpenPopup("not-implemented");
    ImGui::SameLine();
    if (ImGui::Button("Load"))
        ImGui::OpenPopup("not-implemented");

    ImGui::Separator();
    if (ImGui::IsWindowAppearing())
        ImGui::SetKeyboardFocusHere();
    ImGui::PushItemWidth(-0.1f);
    if (ImGui::InputTextWithHint("",
                                 "What do you want to do today?",
                                 s.new_todo_input.data(),
                                 s.input_string_size,
                                 ImGuiInputTextFlags_EnterReturnsTrue)) {
        ctx.dispatch(todo::add_todo_action{s.new_todo_input.data()});
        s.new_todo_input[0] = '\0';
        ImGui::SetKeyboardFocusHere(-1);
    }
    ImGui::PopItemWidth();
    ImGui::Separator();

    ImGui::BeginChild("");
    {
        auto idx = std::size_t{};
        for (auto item : m.todos) {
            ImGui::PushID(idx);
            auto with_idx = [idx](auto&& a) { return std::make_pair(idx, a); };
            draw({ctx, with_idx}, item);
            ImGui::PopID();
            ++idx;
        }
    }
    ImGui::EndChild();

    ImGui::End();
}

int main()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::cerr << "Error initializing SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    const char* glsl_version = "#version 300 es";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    auto current = SDL_DisplayMode{};
    SDL_GetCurrentDisplayMode(0, &current);
    auto window =
        SDL_CreateWindow("Todo Imgui",
                         SDL_WINDOWPOS_CENTERED,
                         SDL_WINDOWPOS_CENTERED,
                         window_width,
                         window_height,
                         SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                             SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Error creating SDL window: " << SDL_GetError()
                  << std::endl;
        return -1;
    }

    auto gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        std::cerr << "Error creating GL context: " << SDL_GetError()
                  << std::endl;
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    auto loop  = lager::sdl_event_loop{};
    auto store = lager::make_store<todo::model_action>(
        todo::model{}, lager::with_sdl_event_loop{loop});
    auto state = ui_state{};

    loop.run(
        [&](const SDL_Event& ev) {
            ImGui_ImplSDL2_ProcessEvent(&ev);
            return ev.type != SDL_QUIT;
        },
        [&](auto dt) {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame(window);
            ImGui::NewFrame();
            {
                draw(store, store.get(), state);
            }
            ImGui::Render();
            SDL_GL_MakeCurrent(window, gl_context);
            auto size = ImGui::GetIO().DisplaySize;
            glViewport(0, 0, (int) size.x, (int) size.y);
            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            SDL_GL_SwapWindow(window);
        });

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
