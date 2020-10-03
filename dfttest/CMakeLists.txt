## The DFT2LNT Compiler project
project (dfttest)
set (dfttest_VERSION_MAJOR 1)
set (dfttest_VERSION_MINOR 3)
set (dfttest_VERSION_PATCH 4)
set (dfttest_VERSION "${dfttest_VERSION_MAJOR}.${dfttest_VERSION_MINOR}.${dfttest_VERSION_PATCH}")

## Not sure if needed
cmake_minimum_required (VERSION 2.6)
if (MSVC)
	add_definitions(/std:c++17)
else()
	add_definitions(-std=c++17)
endif()

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
add_dependencies(dfttest dft2lnt)

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
	dft2lnt
	${YAML_CPP_LIB}
)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

## Installation
install (TARGETS dfttest DESTINATION bin)

# Generate compile time information
set(GENINFOFILE ${CMAKE_CURRENT_BINARY_DIR}/compiletime.h)
get_property(gendepSOURCES TARGET dfttest PROPERTY SOURCES)
LIST(REMOVE_ITEM gendepSOURCES ${GENINFOFILE})
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../.git/HEAD")
	set(GIT_HEAD_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/../.git/HEAD")
	set(NO_GIT 0)
else()
	set(GIT_HEAD_DEPENDENCY "")
	set(NO_GIT 1)
endif()
add_custom_command(
	OUTPUT ${GENINFOFILE}
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../compiletime.cmake
		${gendepSOURCES}
		${GIT_HEAD_DEPENDENCY}
	COMMAND ${CMAKE_COMMAND}
		-DOUTPUT_FILE=${GENINFOFILE}
		-DDFTROOT=${DFTROOT}
		-DFALLBACK_VERSION=${dfttest_VERSION}
		-DNO_GIT=${NO_GIT}
		-P ${CMAKE_CURRENT_SOURCE_DIR}/../compiletime.cmake
)
