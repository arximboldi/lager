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

option(lager_BUILD_TESTS "Build tests" ON)
option(lager_BUILD_EXAMPLES "Build examples" ON)
option(lager_BUILD_DOCS "Build docs" ON)

find_program(CCACHE ccache)
if (CCACHE)
  message(STATUS "Using ccache: ${CCACHE}")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE})
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${CCACHE})
else()
  message(STATUS "Could not find ccache")
endif()

#  Targets
#  =======

# the library
add_library(lager INTERFACE)
target_include_directories(lager INTERFACE
  $<BUILD_INTERFACE:${lager_BINARY_DIR}/>
  $<BUILD_INTERFACE:${lager_SOURCE_DIR}/>
  $<INSTALL_INTERFACE:include>)

install(TARGETS lager EXPORT LagerConfig)

# the library, with http debugger
if (lager_BUILD_TESTS OR lager_BUILD_EXAMPLES)
  find_package(Boost 1.56 COMPONENTS system REQUIRED)
  find_package(Threads REQUIRED)
  find_package(immer REQUIRED)
  find_package(LibHttpServer REQUIRED)
  add_library(lager-debugger INTERFACE)
  target_include_directories(lager-debugger INTERFACE
    $<BUILD_INTERFACE:${lager_BINARY_DIR}/>
    $<BUILD_INTERFACE:${lager_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:include>
    ${Boost_INCLUDE_DIR}
    ${LIBHTTPSERVER_INCLUDE_DIRS})
  target_link_libraries(lager-debugger INTERFACE
    lager
    immer
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_LIBRARIES}
    ${LIBHTTPSERVER_LIBRARIES})

  add_custom_target(gui ALL
    COMMAND make
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/resources/gui"
    COMMENT "Build debugger web UI")

  install(TARGETS lager-debugger EXPORT LagerConfig)

  install(FILES
    resources/gui/gui.js
    resources/gui/gui.css
    resources/gui/index.html
    DESTINATION share/lager/gui)
endif()

# the library, local development target
if(lager_BUILD_TESTS)
  add_library(lager-dev INTERFACE)
  target_include_directories(lager-dev SYSTEM INTERFACE
    "$<BUILD_INTERFACE:${lager_SOURCE_DIR}/>/tools/include")
  target_link_libraries(lager-dev INTERFACE lager)
  if (ENABLE_COVERAGE)
    target_compile_options(lager-dev INTERFACE "--coverage")
    target_link_libraries(lager-dev INTERFACE "--coverage")
  endif()

  enable_testing()
  add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Build and run all the tests and examples.")

  add_subdirectory(test)
endif()

if(lager_BUILD_EXAMPLES)
  add_subdirectory(example)
endif()

if (lager_BUILD_DOCS)
  add_subdirectory(doc)
endif()

install(EXPORT LagerConfig DESTINATION lib/cmake/Lager)
install(DIRECTORY lager DESTINATION include)
