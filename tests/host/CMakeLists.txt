cmake_minimum_required(VERSION 3.16)
project(pdi_host_tests CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

get_filename_component(PDI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

option(PDI_TEST_SANITIZE "Build the host tests with address and UB sanitizers" ON)

# the framework picks the mock device from this gate, leaving the generated
# DeviceSetup.h that selects the user's real board untouched
add_compile_definitions(MOCK_DEVICE_TEST)

include_directories(
  ${PDI_ROOT}
  ${PDI_ROOT}/src
)

add_compile_options(-Wall -Wno-unused-parameter -Wno-unused-variable)

if(PDI_TEST_SANITIZE)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
  add_link_options(-fsanitize=address,undefined)
endif()

# the device translation unit pulls in every mock adapter source it needs
set(PDI_DEVICE_SOURCES ${PDI_ROOT}/src/interface/pdi.cpp)

file(GLOB PDI_UTILITY_SOURCES
  ${PDI_ROOT}/src/utility/*.cpp
  ${PDI_ROOT}/src/utility/queue/*.cpp
)

# portable implementations the device adapters build on, plus the vendored
# littlefs the storage stack runs for real on the emulated flash
file(GLOB PDI_IMPL_SOURCES
  ${PDI_ROOT}/src/interface/pdi/impl/log/*.cpp
  ${PDI_ROOT}/src/interface/pdi/impl/middlewares/*.cpp
  ${PDI_ROOT}/src/interface/pdi/impl/modules/storage/*.cpp
)

# everything above the adapters. the archive only yields the objects something
# references, so the unit tests stay small while pdid can pull the whole stack.
file(GLOB_RECURSE PDI_SERVICE_SOURCES
  ${PDI_ROOT}/src/service_provider/*.cpp
  ${PDI_ROOT}/src/webserver/*.cpp
  ${PDI_ROOT}/src/transports/*.cpp
  ${PDI_ROOT}/src/database/*.cpp
  ${PDI_ROOT}/src/helpers/*.cpp
)

# littlefs is reached the same way the device build reaches it. lfs.c is not a
# translation unit of its own: FileSystemInterfaceImpl.c includes
# LittleFSWrapper.c, which sets LFS_NAME_MAX before including lfs.c. Compiling
# lfs.c directly would leave it on the lfs.h default while the wrapper header
# uses FILE_NAME_MAX_SIZE, and the two sizes of struct lfs_info would disagree.
set(PDI_EXTERNAL_SOURCES
  ${PDI_ROOT}/external/LittleFSWrapper.cpp
  ${PDI_ROOT}/src/interface/pdi/impl/modules/storage/FileSystemInterfaceImpl.c
)

file(GLOB_RECURSE PDI_CRYPTO_SOURCES
  ${PDI_ROOT}/src/utility/crypto/*.cpp
)

# the embedded stl supplies its own support routines. the three files that
# define the c++ abi and unwinding symbols are left out, because the host
# runtime already provides those and two definitions would collide.
file(GLOB PDI_PDISTL_SOURCES ${PDI_ROOT}/src/utility/pdistl/*.cpp)
list(FILTER PDI_PDISTL_SOURCES EXCLUDE REGEX "/(support|eh_globals|eh_alloc)\\.cpp$")

# the framework goes into an archive rather than straight into the executable,
# so the linker pulls only the objects something actually references. that is
# how the arduino build resolves it, and parts of the embedded stl reference
# symbols their own source compiles out.
add_library(pdi_framework STATIC
  ${PDI_UTILITY_SOURCES}
  ${PDI_CRYPTO_SOURCES}
  ${PDI_PDISTL_SOURCES}
  ${PDI_IMPL_SOURCES}
  ${PDI_SERVICE_SOURCES}
  ${PDI_ROOT}/src/PdiStack.cpp
  ${PDI_EXTERNAL_SOURCES}
  ${PDI_DEVICE_SOURCES}
)

target_include_directories(pdi_framework PUBLIC ${PDI_ROOT} ${PDI_ROOT}/src)

file(GLOB PDI_TEST_FRAMEWORK_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/framework/*.cpp
)

file(GLOB PDI_UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/unit/*.cpp
)

# in-process whole-stack tests: the same binary, but reaching through the vfs
# dispatcher and the services rather than one module at a time
file(GLOB PDI_SYSTEM_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/system/*.cpp
)

add_executable(pdi_host_tests
  ${PDI_TEST_FRAMEWORK_SOURCES}
  ${PDI_UNIT_TEST_SOURCES}
  ${PDI_SYSTEM_TEST_SOURCES}
)

target_link_libraries(pdi_host_tests PRIVATE pdi_framework)

# the ed25519 and curve25519 field arithmetic is the upstream ref10 code, which
# shifts negative limbs on purpose. that is defined under the two's complement
# rules C++20 settled on, so the check is dropped for those files only rather
# than the vendored crypto being altered.
file(GLOB_RECURSE PDI_REF10_SOURCES
  ${PDI_ROOT}/src/utility/crypto/asymmetric/ed25519/*.cpp
  ${PDI_ROOT}/src/utility/crypto/asymmetric/curve25519/*.cpp
)

if(PDI_TEST_SANITIZE)
  set_source_files_properties(${PDI_REF10_SOURCES} PROPERTIES
    COMPILE_OPTIONS "-fno-sanitize=shift-base"
  )
endif()

target_include_directories(pdi_host_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/framework)

# the whole stack as a host process, driven by the same initialize/serve pair a
# sketch uses
add_executable(pdid ${CMAKE_CURRENT_SOURCE_DIR}/pdid_main.cpp)
target_link_libraries(pdid PRIVATE pdi_framework)
