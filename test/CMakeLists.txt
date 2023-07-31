
#  Utils
#  =====

find_package(Qt5Core)
find_package(Qt5Concurrent)
find_package(Qt5Qml)
find_package(Qt5Quick)

function(lager_target_name_for out_target out_file file)
  get_filename_component(_extension ${_file} EXT)

  file(RELATIVE_PATH _relative ${PROJECT_SOURCE_DIR} ${file})
  string(REPLACE "${_extension}" "" _name ${_relative})
  string(REGEX REPLACE "/" "-" _name ${_name})
  set(${out_target} "${_name}" PARENT_SCOPE)

  string(REPLACE "${_extension}" "" _name ${_relative})
  string(REGEX REPLACE "/" "-" _name ${_name})
  set(${out_file} "${_name}" PARENT_SCOPE)
endfunction()

function(lager_get_qt_libs out_qt_required out_qt_libs target)
  set(${out_qt_required} FALSE PARENT_SCOPE)
  unset(${out_qt_libs} PARENT_SCOPE)

  string(FIND ${target} "qt" _qt_required)
  if(${_qt_required} GREATER 0)
    set(${out_qt_required} TRUE PARENT_SCOPE)
    if(${Qt5Core_FOUND} AND ${Qt5Concurrent_FOUND})
      set(${out_qt_libs} PRIVATE Qt5::Core Qt5::Concurrent PARENT_SCOPE)
    endif()
  endif()

  string(FIND ${target} "qml" _qml_required)
  if(${_qml_required} GREATER 0)
    set(${out_required} TRUE PARENT_SCOPE)
    if(${Qt5Core_FOUND} AND ${Qt5Quick_FOUND})
      set(${out_qt_libs} PRIVATE Qt5::Core Qt5::Quick PARENT_SCOPE)
    endif()
  endif()
endfunction()

#  Targets
#  =======

find_package(Qt5 COMPONENTS Core Concurrent)

add_custom_target(tests COMMENT "Build all the unit tests.")
add_dependencies(check tests)

file(GLOB lager_unit_tests "*.cpp" "cereal/*.cpp" "event_loop/*.cpp" "extra/*.cpp" "detail/*.cpp")

foreach(_file IN LISTS lager_unit_tests)
  message("found unit test: " ${_file})
  lager_target_name_for(_target _output "${_file}")
  lager_get_qt_libs(_qt_required _qt_libs ${_target})
  if(${_qt_required} AND (NOT DEFINED _qt_libs))
    message(WARNING "Qt dependency not found - skipping target ${_target}")
    continue()
  endif()
  add_executable(${_target} EXCLUDE_FROM_ALL "${_file}")
  set_target_properties(${_target} PROPERTIES OUTPUT_NAME ${_output})
  add_dependencies(tests ${_target})
  target_compile_definitions(${_target} PUBLIC CATCH_CONFIG_MAIN)
  target_link_libraries(${_target} PUBLIC lager-dev Catch2::Catch2 ${_qt_libs})
  add_test("test/${_output}" ${_output})
endforeach()


if (lager_BUILD_FAILURE_TESTS)
  add_subdirectory(build_failure)
endif()
