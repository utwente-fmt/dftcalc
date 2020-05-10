## The DFT2LNT Compiler project
project (dfttest)
set (dfttest_VERSION_MAJOR 1)
set (dfttest_VERSION_MINOR 3)
set (dfttest_VERSION_PATCH 4)

## Not sure if needed
cmake_minimum_required (VERSION 2.6)

add_definitions(-std=c++11)

## Include source directory, the build directory (for generated files) and the dft2lnt library path
include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../dft2lnt
	${CMAKE_CURRENT_SOURCE_DIR}/../dftcalc
)

## Link directories
#link_directories(${CMAKE_CURRENT_BINARY_DIR}/../dft2lnt)

if (DEFINED ENV{CADP})
	add_definitions(-DHAVE_CADP)
endif()

## Specify the executable and its sources
add_executable(dfttest
	dfttest.cpp
	test.cpp
	TestResult.cpp
	TestOutput.cpp
	TestRun.cpp
	${CMAKE_CURRENT_BINARY_DIR}/compiletime.h
)

## Depends on:
##   - dft2lnt (libdft2lnt.a)
add_dependencies(dfttest dft2lntc)

# Find yaml.h on MacOS with Homebrew
find_path(YAML_INCLUDE_PATH "yaml-cpp/yaml.h")
target_include_directories(dfttest PRIVATE ${YAML_INCLUDE_PATH})

## Link with:
##   - libdft2lnt.a
##   - yaml-cpp
find_library(YAML_CPP_LIB yaml-cpp)
if("${YAML_CPP_LIB}" STREQUAL "YAML_CPP_LIB-NOTFOUND")
	set(YAML_CPP_LIB yaml-cpp)
endif()
target_link_libraries(dfttest
	${CMAKE_CURRENT_BINARY_DIR}/../dft2lnt/libdft2lnt.a
	${YAML_CPP_LIB}
	)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

## Installation
install (TARGETS dfttest DESTINATION bin)

# Generate compile time information
set(GENINFOFILE ${CMAKE_CURRENT_BINARY_DIR}/compiletime.h)
get_property(gendepSOURCES TARGET dfttest PROPERTY SOURCES)
LIST(REMOVE_ITEM gendepSOURCES ${GENINFOFILE})
add_custom_command(
	OUTPUT ${GENINFOFILE}
#	TARGET dfttest
#	PRE_BUILD
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../compiletime.cmake ${gendepSOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/../.git/HEAD
	COMMAND ${CMAKE_COMMAND} -DCMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
			-DCMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
			-DDFTROOT=${DFTROOT}
			-DUSECOMMANDLINEGITINFO=${USECOMMANDLINEGITINFO} -Dgit_output=${git_output} -Dgit_changed=${git_changed} -Dgit_version=${git_version} -Dgit_test=${git_test}
			-P ${CMAKE_CURRENT_SOURCE_DIR}/../compiletime.cmake
			../..
)
