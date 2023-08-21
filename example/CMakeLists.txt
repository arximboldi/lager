
add_custom_target(examples COMMENT "Build all examples")
if (lager_BUILD_TESTS)
  add_dependencies(check examples)
endif()

# std  examples
# ====================

add_executable(counter-std EXCLUDE_FROM_ALL
  counter/std/main.cpp)
target_link_libraries(counter-std lager-example)
add_dependencies(examples counter-std)

# ncurses examples
# ================

set(CURSES_NEED_WIDE true)

find_package(Curses)

if(Curses_FOUND)
  if (lager_BUILD_DEBUGGER_EXAMPLES)
    set(counter_ncurses_sources
      counter/ncurses/main.cpp
      counter/ncurses/terminal.cpp)
    set(counter_ncurses_include_directories
      ${CURSES_INCLUDE_DIR})
    set(counter_ncurses_link_libraries
      lager-debugger-example
      ${CURSES_LIBRARIES})

    add_executable(counter-ncurses EXCLUDE_FROM_ALL ${counter_ncurses_sources})
    target_include_directories(counter-ncurses SYSTEM PUBLIC ${counter_ncurses_include_directories})
    target_link_libraries(counter-ncurses ${counter_ncurses_link_libraries})
    target_compile_definitions(counter-ncurses PUBLIC DEBUGGER)
    add_dependencies(examples counter-ncurses)

    add_executable(counter-ncurses-tree EXCLUDE_FROM_ALL ${counter_ncurses_sources})
    target_include_directories(counter-ncurses-tree SYSTEM PUBLIC ${counter_ncurses_include_directories})
    target_link_libraries(counter-ncurses-tree ${counter_ncurses_link_libraries})
    target_compile_definitions(counter-ncurses-tree PUBLIC TREE_DEBUGGER)
    add_dependencies(examples counter-ncurses-tree)

    add_executable(counter-ncurses-meta EXCLUDE_FROM_ALL ${counter_ncurses_sources})
    target_include_directories(counter-ncurses-meta SYSTEM PUBLIC ${counter_ncurses_include_directories})
    target_link_libraries(counter-ncurses-meta ${counter_ncurses_link_libraries})
    target_compile_definitions(counter-ncurses-meta PUBLIC DEBUGGER META_DEBUGGER)
    add_dependencies(examples counter-ncurses-meta)
  endif()
else()
  message(STATUS "Disabling Curses based examples")
endif()

# sdl examples
# ===========

find_package(SDL2)
find_package(SDL2_ttf)

if (SDL2_FOUND AND SDL2_ttf_FOUND)
  if(lager_BUILD_DEBUGGER_EXAMPLES)
    add_executable(counter-sdl2 EXCLUDE_FROM_ALL
      counter/sdl2/main.cpp)
    target_include_directories(counter-sdl2 SYSTEM PUBLIC
      ${SDL2_INCLUDE_DIRS}
      ${SDL2_TTF_INCLUDE_DIR})
    target_link_libraries(counter-sdl2
      lager-debugger-example
      ${SDL2_LIBRARIES}
      ${SDL2_TTF_LIBRARIES})
    add_dependencies(examples counter-sdl2)

    add_executable(autopong EXCLUDE_FROM_ALL
      autopong/autopong.cpp
      autopong/sdl2/main.cpp)
    target_include_directories(autopong SYSTEM PUBLIC
      ${SDL2_INCLUDE_DIRS}
      ${SDL2_TTF_INCLUDE_DIR})
    target_link_libraries(autopong
      lager-debugger-example
      ${SDL2_LIBRARIES}
      ${SDL2_TTF_LIBRARIES})
    add_dependencies(examples autopong)

    add_executable(autopong-debug EXCLUDE_FROM_ALL
      autopong/autopong.cpp
      autopong/sdl2/main.cpp)
    target_compile_definitions(autopong-debug PUBLIC DEBUGGER)
    target_include_directories(autopong-debug SYSTEM PUBLIC
      ${SDL2_INCLUDE_DIRS}
      ${SDL2_TTF_INCLUDE_DIR})
    target_link_libraries(autopong-debug
      lager-debugger-example
      ${SDL2_LIBRARIES}
      ${SDL2_TTF_LIBRARIES})
    add_dependencies(examples autopong-debug)
  endif()
else()
  message(STATUS "Disabling SDL based examples")
endif()

# qt examples
# ===========

find_package(Qt5Core)
find_package(Qt5Concurrent)
find_package(Qt5Qml)
find_package(Qt5Gui)
find_package(Qt5Widgets)
find_package(Qt5QuickControls2)

find_package(Boost 1.56)

if (Qt5Core_FOUND AND Qt5Concurrent_FOUND AND Qt5Gui_FOUND AND Qt5Widgets_FOUND AND Qt5Qml_FOUND AND Qt5QuickControls2_FOUND AND Boost_FOUND)
  add_executable(todo-qml EXCLUDE_FROM_ALL
    todo/item.cpp
    todo/model.cpp
    todo/qml/main.cpp)
  set_target_properties(todo-qml PROPERTIES AUTOMOC YES)
  target_link_libraries(todo-qml lager-example Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Qml Qt5::QuickControls2)
  target_compile_definitions(todo-qml PRIVATE
    LAGER_TODO_QML_DIR="${CMAKE_CURRENT_SOURCE_DIR}/todo/qml")
  add_dependencies(examples todo-qml)

  add_executable(todo-qml-redux EXCLUDE_FROM_ALL
    todo/item.cpp
    todo/model.cpp
    todo/app.cpp
    todo/qml-redux/main.cpp)
  set_target_properties(todo-qml-redux PROPERTIES AUTOMOC YES)
  target_link_libraries(todo-qml-redux lager-example Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Qml Qt5::QuickControls2)
  target_compile_definitions(todo-qml-redux PRIVATE
    LAGER_TODO_QML_DIR="${CMAKE_CURRENT_SOURCE_DIR}/todo/qml-redux")
  add_dependencies(examples todo-qml-redux)

  add_executable(snake-qml EXCLUDE_FROM_ALL
    snake/model.cpp snake/qml/main.cpp snake/qml/qmodel.cpp)
  set_target_properties(snake-qml PROPERTIES AUTOMOC YES)
  target_link_libraries(snake-qml lager-example Qt5::Core Qt5::Concurrent Qt5::Gui Qt5::Widgets Qt5::Qml Qt5::QuickControls2)
  target_include_directories(snake-qml PRIVATE ${Boost_INCLUDE_DIRS})
  target_compile_definitions(snake-qml PRIVATE
    LAGER_SNAKE_QML_DIR="${CMAKE_CURRENT_SOURCE_DIR}/snake/qml")
  add_dependencies(examples snake-qml)
else()
  message(STATUS "Disabling Qt based examples")
endif()

# ImGui examples
# ==============

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL)

set(imgui_source_dir "$ENV{IMGUI_SOURCE_DIR}")

set(imgui_sources
  "${imgui_source_dir}/imgui.cpp"
  "${imgui_source_dir}/imgui_draw.cpp"
  "${imgui_source_dir}/imgui_widgets.cpp"
  "${imgui_source_dir}/imgui_impl_sdl.cpp"
  "${imgui_source_dir}/imgui_impl_opengl3.cpp")

find_program(EMXX em++)

if (EMXX AND OPENGL_FOUND AND SDL2_FOUND AND EXISTS "${imgui_source_dir}/imgui.h")
  message(STATUS "ImGui found in ${imgui_source_dir}")

  set(extra_emcc_flags $ENV{NIX_CFLAGS_COMPILE})
  separate_arguments(extra_emcc_flags)

  add_custom_target(todo-imgui
    COMMAND ${EMXX} ${extra_emcc_flags}
      -Os
      -std=c++17 -I ${CMAKE_SOURCE_DIR}
      -I ${imgui_source_dir}
      ${imgui_sources}
      ${CMAKE_CURRENT_SOURCE_DIR}/todo/item.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/todo/model.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/todo/imgui/main.cpp
      -s USE_SDL=2 -s USE_WEBGL2=1 -s WASM=1 -s FULL_ES3=1
      -s ALLOW_MEMORY_GROWTH=1
      --emrun
      --shell-file ${CMAKE_SOURCE_DIR}/resources/emscripten_shell_minimal.html
      -o ${CMAKE_CURRENT_BINARY_DIR}/todo-imgui.html
    COMMENT "Build todo-imgui emscripten example")
  add_dependencies(examples todo-imgui)
else()
  message(STATUS "Disabling ImGui based examples")
endif()
