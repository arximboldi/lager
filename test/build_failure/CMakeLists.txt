include(icm_build_failure_testing)

set(_glob_pattern "*.cpp")

file(GLOB_RECURSE lager_build_failure_tests ${_glob_pattern})
foreach(_file IN LISTS lager_build_failure_tests)
  message("found build failure test: " ${_file})
endforeach()

# We test that the build fails and the failure message
# contains the offending line.
#
# see: https://ibob.bg/blog/2022/10/04/testing-build-failure-with-cmake/
icm_glob_build_failure_tests(
  PATTERN ${_glob_pattern}
  LIBRARIES lager-dev
  PREFIX "build-failure"
  ERROR_MATCHES "offending line"
)
