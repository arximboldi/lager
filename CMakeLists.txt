cmake_minimum_required(VERSION 3.8)
cmake_policy(SET CMP0048 NEW) # enable project VERSION
cmake_policy(SET CMP0056 NEW) # honor link flags in try_compile()
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(lager VERSION 0.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_EXPORT_COMPILE_COMMANDS on)
set(CMAKE_CXX_EXTENSIONS off)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Qunused-arguments")
endif()

#  Targets
#  =======

# the library
add_library(lager INTERFACE)
target_include_directories(lager INTERFACE
  $<BUILD_INTERFACE:${lager_BINARY_DIR}/>
  $<BUILD_INTERFACE:${lager_SOURCE_DIR}/>
  $<INSTALL_INTERFACE:include>)

# the library, local development target
add_library(lager-dev INTERFACE)
target_include_directories(lager-dev SYSTEM INTERFACE
  "$<BUILD_INTERFACE:${lager_SOURCE_DIR}/>/tools/include")
target_link_libraries(lager-dev INTERFACE
  lager)
if (ENABLE_COVERAGE)
  target_compile_options(immer-dev INTERFACE "--coverage")
  target_link_libraries(immer-dev INTERFACE "--coverage")
endif()

add_custom_target(gui ALL
  COMMAND make
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/resources/gui"
  COMMENT "Build debugger web UI")

configure_file(lager/config.hpp.in
  "${CMAKE_SOURCE_DIR}/lager/config.hpp")

install(TARGETS lager EXPORT LagerConfig)
install(EXPORT LagerConfig DESTINATION lib/cmake/Lager)
install(DIRECTORY lager DESTINATION include)
install(FILES
  resources/gui/gui.js
  resources/gui/gui.css
  resources/gui/index.html
  DESTINATION share/lager/gui)

enable_testing()
add_custom_target(check
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Build and run all the tests and examples.")

#  Subdirs
#  =======

add_subdirectory(example)
add_subdirectory(test)
