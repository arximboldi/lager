
set(CMAKE_CURSES_NEED_WIDE true)

find_package(Curses)
find_package(Boost 1.56 COMPONENTS system)
find_package(Threads)

add_executable(example-counter-ncurses EXCLUDE_FROM_ALL
  main.cpp
  terminal.cpp)
target_include_directories(example-counter-ncurses SYSTEM PUBLIC
  ${CURSES_INCLUDE_DIR}
  ${Boost_INCLUDE_DIR})
target_link_libraries(example-counter-ncurses
  lager
  ${CURSES_LIBRARIES}
  ${Boost_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT})
