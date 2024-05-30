cmake_minimum_required(VERSION 3.12)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

# project information
project(unit_tests
        VERSION 0.1
	      DESCRIPTION "Unit tests for Ledger Nano application"
        LANGUAGES C)


# guard against bad build-type strings
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

include(CTest)
ENABLE_TESTING()

# specify C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -pedantic -g -O0 --coverage")

set(GCC_COVERAGE_LINK_FLAGS "--coverage -lgcov")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")

# guard against in-source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt. ")
endif()

add_compile_definitions(TEST)

# force include macro helpers into all files
add_definitions(-include macro_helpers.h)

include_directories(BEFORE SYSTEM ../src utils)

file(GLOB SHIMS_SRC utils/*.c)

# CX library shim for testing
add_library(sdk_shims SHARED ${SHIMS_SRC})

add_library(bip32_ext SHARED ../src/common/bip32_ext.c)
add_library(rwbuffer SHARED ../src/common/buffer_ext.c ../src/common/rwbuffer.c)
add_library(gve SHARED ../src/common/gve.c)
add_library(blake2b SHARED ../src/helpers/blake2b.c)
add_library(ergo_tree SHARED ../src/ergo/ergo_tree.c)
add_library(tx_ser_box SHARED ../src/ergo/tx_ser_box.c)
add_library(tx_ser_full SHARED ../src/ergo/tx_ser_full.c)
add_library(tx_ser_input SHARED ../src/ergo/tx_ser_input.c)
add_library(tx_ser_table SHARED ../src/ergo/tx_ser_table.c)
add_library(address SHARED ../src/ergo/address.c)
add_library(input_frame SHARED ../src/helpers/input_frame.c)

target_link_libraries(bip32_ext PUBLIC sdk_shims)
target_link_libraries(blake2b PUBLIC sdk_shims)
target_link_libraries(rwbuffer PUBLIC bip32_ext)
target_link_libraries(gve PUBLIC rwbuffer)
target_link_libraries(address PUBLIC rwbuffer blake2b)
target_link_libraries(input_frame PUBLIC rwbuffer)
target_link_libraries(ergo_tree PUBLIC rwbuffer)
target_link_libraries(tx_ser_table PUBLIC blake2b rwbuffer gve)
target_link_libraries(tx_ser_input PUBLIC rwbuffer blake2b tx_ser_table)
target_link_libraries(tx_ser_box PUBLIC blake2b rwbuffer tx_ser_table gve ergo_tree)
target_link_libraries(tx_ser_full PUBLIC blake2b rwbuffer gve tx_ser_box tx_ser_input tx_ser_table)

add_executable(test_address test_address.c)
add_executable(test_bip32 test_bip32.c)
add_executable(test_buffer test_buffer.c)
add_executable(test_ergo_tree test_ergo_tree.c)
add_executable(test_full_tx test_full_tx.c)
add_executable(test_gve test_gve.c)
add_executable(test_input_frame test_input_frame.c)
add_executable(test_safeint test_safeint.c)
add_executable(test_tx_ser_box test_tx_ser_box.c)
add_executable(test_tx_ser_input test_tx_ser_input.c)
add_executable(test_tx_ser_table test_tx_ser_table.c)
add_executable(test_zigzag test_zigzag.c)

target_link_libraries(test_address PUBLIC cmocka gcov address)
target_link_libraries(test_bip32 PUBLIC cmocka gcov bip32_ext)
target_link_libraries(test_buffer PUBLIC cmocka gcov rwbuffer)
target_link_libraries(test_ergo_tree PUBLIC cmocka gcov ergo_tree)
target_link_libraries(test_full_tx PUBLIC cmocka gcov blake2b tx_ser_full)
target_link_libraries(test_gve PUBLIC cmocka gcov gve)
target_link_libraries(test_input_frame PUBLIC cmocka gcov input_frame)
target_link_libraries(test_safeint PUBLIC cmocka gcov)
target_link_libraries(test_tx_ser_box PUBLIC cmocka gcov tx_ser_box)
target_link_libraries(test_tx_ser_input PUBLIC cmocka gcov tx_ser_input)
target_link_libraries(test_tx_ser_table PUBLIC cmocka gcov tx_ser_table)
target_link_libraries(test_zigzag PUBLIC cmocka gcov)

add_test(test_address test_address)
add_test(test_bip32 test_bip32)
add_test(test_buffer test_buffer)
add_test(test_ergo_tree test_ergo_tree)
add_test(test_full_tx test_full_tx)
add_test(test_gve test_gve)
add_test(test_input_frame test_input_frame)
add_test(test_safeint test_safeint)
add_test(test_tx_ser_box test_tx_ser_box)
add_test(test_tx_ser_input test_tx_ser_input)
add_test(test_tx_ser_table test_tx_ser_table)
add_test(test_zigzag test_zigzag)
