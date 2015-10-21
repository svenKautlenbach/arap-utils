cmake_minimum_required (VERSION 3.2.2)
project (arap-utils-test)

set (CMAKE_CXX_FLAGS "-Wall -Werror -std=gnu++11")
set (CMAKE_EXE_LINKER_FLAGS "-pthread")

include_directories (
	"${PROJECT_SOURCE_DIR}"
	"${PROJECT_SOURCE_DIR}/gtest/"
	"${PROJECT_SOURCE_DIR}/../"
	)

file (GLOB SOURCES "test-main.cpp" "timer-test.cpp" "../ArapUtils.cpp")

add_executable (arap-utils-test ${SOURCES})
target_link_libraries (arap-utils-test ${PROJECT_SOURCE_DIR}/gtest/libgtest.a)
