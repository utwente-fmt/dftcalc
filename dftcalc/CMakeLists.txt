## The DFT2LNT Compiler project
project (dftcalc)
set (dftcalc_VERSION_MAJOR 1)
set (dftcalc_VERSION_MINOR 3)
set (dftcalc_VERSION_PATCH 4)
set (dftcalc_VERSION "${dftcalc_VERSION_MAJOR}.${dftcalc_VERSION_MINOR}.${dftcalc_VERSION_PATCH}")

## Not sure if needed
cmake_minimum_required (VERSION 3.0)

if (MSVC)
	add_definitions(/std:c++17)
else()
	add_definitions(-std=c++17)
endif()

if(DEFINED ENV{CADP})
	add_definitions(-DHAVE_CADP)
endif()

## Include source directory, the build directory (for generated files) and the dft2lnt library path
include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../dft2lnt
)

## Specify the executable and its sources
add_executable(dftcalc
	dftcalc.cpp
	imca.cpp
	storm.cpp
	executor.cpp
	mrmc.cpp
	modest.cpp
	${CMAKE_CURRENT_BINARY_DIR}/compiletime.h
)

## Depends on:
##   - dft2lnt (libdft2lnt.a)
add_dependencies(dftcalc dft2lnt)

# Find yaml.h on MacOS with Homebrew
find_path(YAML_INCLUDE_PATH "yaml-cpp/yaml.h")
target_include_directories(dftcalc PRIVATE ${YAML_INCLUDE_PATH})

## Link with:
##   - libdft2lnt.a
##   - yaml-cpp
find_library(YAML_CPP_LIB yaml-cpp)
if("${YAML_CPP_LIB}" STREQUAL "YAML_CPP_LIB-NOTFOUND")
	set(YAML_CPP_LIB yaml-cpp)
endif()
target_link_libraries(dftcalc dft2lnt ${YAML_CPP_LIB})

if (WIN32)
target_link_libraries(dftcalc pathcch)
endif()

if(UNIX AND NOT APPLE AND NOT WIN32)
	target_link_libraries(dftcalc rt)
endif()

## POST_BUILD: copy the executable to the bin/ folder
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

## Installation
install (TARGETS dftcalc DESTINATION bin)

# Generate compile time information
set(GENINFOFILE ${CMAKE_CURRENT_BINARY_DIR}/compiletime.h)
get_property(gendepSOURCES TARGET dftcalc PROPERTY SOURCES)
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
		-DFALLBACK_VERSION=${dftcalc_VERSION}
		-DNO_GIT=${NO_GIT}
		-P ${CMAKE_CURRENT_SOURCE_DIR}/../compiletime.cmake
)
