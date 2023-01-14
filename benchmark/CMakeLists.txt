
#  Config
#  ======

option(CHECK_BENCHMARKS "Run benchmarks on check target" on)

set(BENCHMARK_PARAM   "N:1000" CACHE STRING "Benchmark parameters")
set(BENCHMARK_SAMPLES "20"     CACHE STRING "Benchmark samples")

#  Dependencies
#  ============

# These are expected to be in the include path, the nix-shell
# environment installs them:
#
#    https://github.com/marcusz/steady
#    https://github.com/deepsea-inria/chunkedseq.git
#    https://github.com/rsms/immutable-cpp.git

#  Targets
#  =======

add_custom_target(benchmarks
  COMMENT "Build all benchmarks.")

execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE lager_git_commit_hash
  OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(
  COMMAND git status --porcelain
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE lager_git_status
  OUTPUT_STRIP_TRAILING_WHITESPACE)

if (NOT lager_git_status STREQUAL "")
  set(lager_git_commit_hash "${lager_git_commit_hash}+")
endif()

site_name(lager_hostname)

get_filename_component(lager_compiler_name "${CMAKE_CXX_COMPILER}" NAME)

set(lager_benchmark_report_base_dir "${CMAKE_SOURCE_DIR}/reports")
set(lager_benchmark_report_dir "${lager_benchmark_report_base_dir}/report_\
${lager_git_commit_hash}_\
${lager_hostname}_\
${lager_compiler_name}_\
${BENCHMARK_PARAM}_\
s${BENCHMARK_SAMPLES}")

if(CHECK_BENCHMARKS)
  add_dependencies(check benchmarks)
endif()

add_custom_target(benchmark-report-dir
  COMMAND ${CMAKE_COMMAND}
  -E make_directory ${lager_benchmark_report_dir})

file(GLOB_RECURSE lager_benchmarks "*.cpp")
foreach(_file IN LISTS lager_benchmarks)
  lager_target_name_for(_target _output "${_file}")
  add_executable(${_target} EXCLUDE_FROM_ALL "${_file}")
  set_target_properties(${_target} PROPERTIES OUTPUT_NAME ${_output})
  add_dependencies(benchmarks ${_target})
  add_dependencies(${_target} benchmark-report-dir)
  target_compile_options(${_target} PUBLIC -Wno-unused-function)
  target_compile_definitions(${_target} PUBLIC NONIUS_RUNNER)
  target_link_libraries(${_target} PUBLIC lager-dev)
  if(CHECK_BENCHMARKS)
    add_test("benchmark/${_output}" "${CMAKE_SOURCE_DIR}/tools/with-tee.bash"
      ${lager_benchmark_report_dir}/${_target}.out
      "${CMAKE_CURRENT_BINARY_DIR}/${_output}" -v
      -t ${_target}
      -r html
      -s ${BENCHMARK_SAMPLES}
      -p ${BENCHMARK_PARAM}
      -o ${lager_benchmark_report_dir}/${_target}.html)
  endif()
endforeach()

set(lager_ssh_method
  ssh -p 5488
      -o StrictHostKeyChecking=no)

add_custom_target(upload-benchmark-reports
  COMMAND
  rsync -av -e \"${lager_ssh_method}\"
        ${lager_benchmark_report_base_dir}
        raskolnikov@sinusoid.es:public/misc/lager/)

add_custom_target(copy-benchmark-reports
  COMMAND
  rsync -av ${lager_benchmark_report_base_dir}
        ~/public/misc/lager/)
