
#  Utils
#  =====

function(lager_target_name_for out_target out_file file)
  get_filename_component(_extension ${_file} EXT)

  string(REPLACE "${_extension}" "" _name ${file})
  string(REGEX REPLACE "/" "-" _name ${_name})
  set(${out_target} "test-${_name}" PARENT_SCOPE)

  string(REPLACE "${_extension}" "" _name ${file})
  string(REGEX REPLACE "/" "-" _name ${_name})
  set(${out_file} "${_name}" PARENT_SCOPE)
endfunction()

#  Targets
#  =======

add_custom_target(tests COMMENT "Build all the unit tests.")
add_dependencies(check tests)

file(GLOB_RECURSE lager_unit_tests "*.cpp")
set(lager_unit_tests
  core.cpp
  deps.cpp
  detail/signals.cpp)

if(lager_BUILD_DEBUGGER)
  list(APPEND lager_unit_tests
    debug.cpp
    cereal/immer_array.cpp
    cereal/immer_box.cpp
    cereal/immer_flex_vector.cpp
    cereal/immer_vector.cpp
    cereal/struct.cpp
    cereal/tuple.cpp)
endif()

foreach(_file IN LISTS lager_unit_tests)
  message("found unit test: " ${_file})
  lager_target_name_for(_target _output "${_file}")
  add_executable(${_target} EXCLUDE_FROM_ALL "${_file}")
  set_target_properties(${_target} PROPERTIES OUTPUT_NAME ${_output})
  add_dependencies(tests ${_target})
  target_compile_definitions(${_target} PUBLIC CATCH_CONFIG_MAIN)
  target_link_libraries(${_target} PUBLIC lager-dev)
  add_test("test/${_output}" ${_output})
endforeach()
