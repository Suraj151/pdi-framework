# the fuzz harnesses. these need clang: libfuzzer ships with the clang runtime
# and gcc has no equivalent, so the whole tree has to be built by the same
# compiler that instruments it.
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  message(FATAL_ERROR "the fuzz targets need clang; configure with -DCMAKE_CXX_COMPILER=clang++")
endif()

# the framework is rebuilt here rather than reused, because coverage
# instrumentation has to reach the parsers under test and not just the harness
add_compile_options(-fsanitize=fuzzer-no-link -fno-omit-frame-pointer -g)

add_library(pdi_framework_fuzz STATIC
  ${PDI_UTILITY_SOURCES}
  ${PDI_CRYPTO_SOURCES}
  ${PDI_PDISTL_SOURCES}
  ${PDI_IMPL_SOURCES}
  ${PDI_SERVICE_SOURCES}
  ${PDI_ROOT}/src/PdiStack.cpp
  ${PDI_EXTERNAL_SOURCES}
  ${PDI_DEVICE_SOURCES}
)

target_include_directories(pdi_framework_fuzz PUBLIC ${PDI_ROOT} ${PDI_ROOT}/src)

# the same exemption the host tests make: ref10 shifts negative limbs on
# purpose, which the ub sanitizer flags and c++20 defines
if(PDI_TEST_SANITIZE)
  set_source_files_properties(${PDI_REF10_SOURCES} PROPERTIES
    COMPILE_OPTIONS "-fno-sanitize=shift-base"
  )
endif()

set(PDI_FUZZ_TARGETS
  fuzz_ssh_wire
  fuzz_sftp
  fuzz_http
  fuzz_http_response
  fuzz_shell
  fuzz_dbrecord
  fuzz_config
  fuzz_crontab
  fuzz_mdns
  fuzz_json
  fuzz_mqtt
)

foreach(target ${PDI_FUZZ_TARGETS})
  add_executable(${target} ${CMAKE_CURRENT_SOURCE_DIR}/${target}.cpp)
  target_include_directories(${target} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PDI_ROOT}/tests/host/framework
  )
  target_link_libraries(${target} PRIVATE pdi_framework_fuzz)
  target_link_options(${target} PRIVATE -fsanitize=fuzzer)
endforeach()
