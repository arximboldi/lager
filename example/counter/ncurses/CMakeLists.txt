
set(CMAKE_CURSES_NEED_WIDE true)

find_package(Curses)
find_package(Boost 1.56 COMPONENTS system)
find_package(Threads)
find_package(Immer)
find_package(LibHttpServer)

add_executable(example-counter-ncurses EXCLUDE_FROM_ALL
  main.cpp
  terminal.cpp)
target_include_directories(example-counter-ncurses SYSTEM PUBLIC
  ${CURSES_INCLUDE_DIR}
  ${Boost_INCLUDE_DIR}
  ${LIBHTTPSERVER_INCLUDE_DIRS})
target_link_libraries(example-counter-ncurses
  lager
  immer
  ${CMAKE_THREAD_LIBS_INIT}
  ${CURSES_LIBRARIES}
  ${Boost_LIBRARIES}
  ${LIBHTTPSERVER_LIBRARIES})
