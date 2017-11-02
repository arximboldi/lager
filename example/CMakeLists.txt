
add_custom_target(examples COMMENT "Build all examples")
add_dependencies(check examples)

# std examples
# ============

add_executable(counter-std EXCLUDE_FROM_ALL
  counter/std/main.cpp)
target_link_libraries(counter-std lager)
add_dependencies(examples counter-std)

# ncurses examples
# ================

set(CMAKE_CURSES_NEED_WIDE true)
find_package(Curses)
find_package(Boost 1.56 COMPONENTS system)
find_package(Threads)
find_package(Immer)
find_package(LibHttpServer)

set(counter_ncurses_sources
  counter/ncurses/main.cpp
  counter/ncurses/terminal.cpp)
set(counter_ncurses_include_directories
  ${CURSES_INCLUDE_DIR}
  ${Boost_INCLUDE_DIR}
  ${LIBHTTPSERVER_INCLUDE_DIRS})
set(counter_ncurses_link_libraries
  lager
  immer
  ${CMAKE_THREAD_LIBS_INIT}
  ${CURSES_LIBRARIES}
  ${Boost_LIBRARIES}
  ${LIBHTTPSERVER_LIBRARIES})

add_executable(counter-ncurses EXCLUDE_FROM_ALL ${counter_ncurses_sources})
target_include_directories(counter-ncurses SYSTEM PUBLIC ${counter_ncurses_include_directories})
target_link_libraries(counter-ncurses ${counter_ncurses_link_libraries})
target_compile_definitions(counter-ncurses PUBLIC DEBUGGER)
add_dependencies(examples counter-ncurses)

add_executable(counter-ncurses-meta EXCLUDE_FROM_ALL ${counter_ncurses_sources})
target_include_directories(counter-ncurses-meta SYSTEM PUBLIC ${counter_ncurses_include_directories})
target_link_libraries(counter-ncurses-meta ${counter_ncurses_link_libraries})
target_compile_definitions(counter-ncurses-meta PUBLIC DEBUGGER META_DEBUGGER)
add_dependencies(examples counter-ncurses-meta)
