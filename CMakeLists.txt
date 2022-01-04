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
option(lager_BUILD_DEBUGGER "Build web based debugger" ON)
option(lager_BUILD_DOCS "Build docs" ON)
option(lager_EMBED_RESOURCES_PATH "Embed installation paths for easier, non-portable resource location" ON)

if (NOT lager_EMBED_RESOURCES_PATH AND lager_BUILD_EXAMPLES)
  message(FATAL_ERROR "Examples require embedded resources path")
endif()

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
  find_package(Immer REQUIRED)

  add_library(lager-example INTERFACE)
  target_include_directories(lager-example INTERFACE
     $<BUILD_INTERFACE:${lager_BINARY_DIR}/>
     $<BUILD_INTERFACE:${lager_SOURCE_DIR}/>
     $<INSTALL_INTERFACE:include>
    ${Boost_INCLUDE_DIR}
  )
  target_link_libraries(lager-example INTERFACE
    lager
    immer
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_LIBRARIES}
  )

  if (lager_BUILD_DEBUGGER)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBHTTPSERVER REQUIRED libhttpserver)
    add_library(lager-debugger INTERFACE)
    target_include_directories(lager-debugger INTERFACE
      $<BUILD_INTERFACE:${lager_BINARY_DIR}/>
      $<BUILD_INTERFACE:${lager_SOURCE_DIR}/>
      $<INSTALL_INTERFACE:include>
      ${Boost_INCLUDE_DIR}
      ${LIBHTTPSERVER_INCLUDE_DIRS})
    target_link_libraries(lager-debugger INTERFACE
      lager-example
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
endif()

# the library, local development target
if(lager_BUILD_TESTS)
  add_library(lager-dev INTERFACE)
  target_include_directories(lager-dev SYSTEM INTERFACE
    "$<BUILD_INTERFACE:${lager_SOURCE_DIR}/>/tools/include"
    ${Boost_INCLUDE_DIR}
  )
  target_link_libraries(lager-dev INTERFACE lager
    ${Boost_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT})
  if (ENABLE_COVERAGE)
    target_compile_options(lager-dev INTERFACE "--coverage")
    target_link_libraries(lager-dev INTERFACE "--coverage")
  endif()
  if (ENABLE_ASAN)
    target_compile_options(lager-dev INTERFACE
      -fno-omit-frame-pointer -fsanitize=address)
    target_link_options(lager-dev INTERFACE -fsanitize=address)
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

# Also configure and install the resources_path.hpp file if wanted
if (lager_EMBED_RESOURCES_PATH)
  set(LAGER_PREFIX_PATH ${CMAKE_INSTALL_PREFIX})
  configure_file(lager/resources_path.hpp.in
    ${CMAKE_BINARY_DIR}/include/lager/resources_path.hpp)
  target_include_directories(lager
    INTERFACE $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>)
  install(FILES ${CMAKE_BINARY_DIR}/include/lager/resources_path.hpp DESTINATION include/lager)
endif()

install(EXPORT LagerConfig DESTINATION lib/cmake/Lager)
install(DIRECTORY lager DESTINATION include PATTERN "*.hpp")
