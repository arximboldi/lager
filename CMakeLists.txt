cmake_minimum_required(VERSION 3.8)
cmake_policy(SET CMP0048 NEW) # enable project VERSION
cmake_policy(SET CMP0056 NEW) # honor link flags in try_compile()
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(lager VERSION 0.1.0)

if(MSVC)
  set(CMAKE_CXX_STANDARD 20)
  add_compile_options(/Wall /Zc:preprocessor /bigobj)
else()
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS on)
set(CMAKE_CXX_EXTENSIONS off)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Qunused-arguments")
endif()

include(GNUInstallDirs)

option(lager_BUILD_TESTS "Build tests" ON)
option(lager_BUILD_FAILURE_TESTS "Build failure tests" ON)
option(lager_BUILD_EXAMPLES "Build examples" ON)
option(lager_BUILD_DEBUGGER_EXAMPLES "Build examples that showcase the web based debugger" ON)
option(lager_BUILD_DOCS "Build docs" ON)
option(lager_EMBED_RESOURCES_PATH "Embed installation paths for easier, non-portable resource location" ON)
option(lager_DISABLE_STORE_DEPENDENCY_CHECKS "Disable compile-time checks for store dependencies" OFF)

if (NOT lager_EMBED_RESOURCES_PATH AND lager_BUILD_EXAMPLES)
  message(FATAL_ERROR "Examples require embedded resources path")
endif()

if (MSVC AND NOT lager_DISABLE_STORE_DEPENDENCY_CHECKS)
  message(WARNING "compile-time checks for store dependencies are currently not supported with msvc")
  set(lager_DISABLE_STORE_DEPENDENCY_CHECKS ON)
endif()

if (NOT lager_BUILD_EXAMPLES AND lager_BUILD_DEBUGGER_EXAMPLES)
    message(WARNING "examples using the web-based debugger are disabled when examples are disabled")
    set(lager_BUILD_DEBUGGER_EXAMPLES OFF)
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
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

if(lager_DISABLE_STORE_DEPENDENCY_CHECKS)
  message(STATUS "Disabling compile-time checks for store dependencies")
  target_compile_definitions(lager INTERFACE LAGER_DISABLE_STORE_DEPENDENCY_CHECKS)
endif()

install(TARGETS lager EXPORT LagerConfig)

# requirements for tests and examples
if (lager_BUILD_TESTS OR lager_BUILD_EXAMPLES)
  find_package(Boost 1.56 COMPONENTS system REQUIRED)
  find_package(Threads REQUIRED)
  find_package(Immer REQUIRED)
  find_package(Zug REQUIRED)
endif()

# the library, local development target
if(lager_BUILD_TESTS)
  find_package(Catch2 REQUIRED)

  add_library(lager-dev INTERFACE)
  target_include_directories(lager-dev SYSTEM INTERFACE
    ${lager_SOURCE_DIR}
    ${Boost_INCLUDE_DIR}
  )
  target_link_libraries(lager-dev INTERFACE lager
    ${Boost_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    immer
    zug
  )
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

# the library, with http debugger
if (lager_BUILD_EXAMPLES)

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

  install(TARGETS lager-example EXPORT LagerConfig)

  # examples with the http debugger
  if (lager_BUILD_DEBUGGER_EXAMPLES)
    add_library(lager-debugger-example INTERFACE)
    target_link_libraries(lager-debugger-example INTERFACE
      lager-example
      Boost::system)

    if (NOT APPLE)
      add_custom_target(gui ALL
        COMMAND make
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/resources/gui"
        COMMENT "Build debugger web UI")
    endif()

    install(TARGETS lager-debugger-example  EXPORT LagerConfig)

    install(FILES
      resources/gui/gui.js
      resources/gui/gui.css
      resources/gui/index.html
      DESTINATION "${CMAKE_INSTALL_DATADIR}/lager/gui")
    endif()

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
  install(FILES ${CMAKE_BINARY_DIR}/include/lager/resources_path.hpp DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/lager")
endif()

install(EXPORT LagerConfig DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Lager")
install(DIRECTORY lager DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}" PATTERN "*.hpp")
