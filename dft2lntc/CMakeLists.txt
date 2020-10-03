## The DFT2LNT Compiler project
project (dft2lntc)
set (dft2lntc_VERSION_MAJOR 1)
set (dft2lntc_VERSION_MINOR 3)
set (dft2lntc_VERSION_PATCH 4)
set (dft2lntc_VERSION "${dft2lntc_VERSION_MAJOR}.${dft2lntc_VERSION_MINOR}.${dft2lntc_VERSION_PATCH}")

## Not sure if needed
cmake_minimum_required (VERSION 2.6)

if (MSVC)
	add_compile_options(/std:c++17)
else()
	add_compile_options(-std=c++17)
endif()

if (DEFINED ENV{CADP})
	add_definitions(-DHAVE_CADP)
endif()

## Include source directory, the build directory (for generated files) and the dft2lnt library path
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../dft2lnt)

if (DEFINED ENV{CADP})
	set(ADDITIONAL_SOURCES DFTreeBCGNodeBuilder.cpp)
else()
	set(ADDITIONAL_SOURCES)
endif()


## Specify the executable and its sources
add_executable(dft2lntc
	dft2lntc.cpp
	modularize.cpp
	DFTreeAUTNodeBuilder.cpp
	DFTreeEXPBuilder.cpp
	DFTreeNodeBuilder.cpp
	${CMAKE_CURRENT_BINARY_DIR}/compiletime.h
	automata/automata.cpp
	automata/automaton.cpp
	automata/be.cpp
	automata/fdep.cpp
	automata/insp.cpp
	automata/spare.cpp
	automata/voting.cpp
	automata/pand.cpp
	${ADDITIONAL_SOURCES}
)

## Depends on:
##   - dft2lnt (libdft2lnt.a)
add_dependencies(dft2lntc dft2lnt)

# Find yaml.h on MacOS with Homebrew
find_path(YAML_INCLUDE_PATH "yaml-cpp/yaml.h")
target_include_directories(dft2lntc PRIVATE ${YAML_INCLUDE_PATH})

find_library(YAML_CPP_LIB yaml-cpp)
if("${YAML_CPP_LIB}" STREQUAL "YAML_CPP_LIB-NOTFOUND")
	set(YAML_CPP_LIB yaml-cpp)
endif()

## Link with:
##   - libdft2lnt.a
##   - yaml-cpp
target_link_libraries(dft2lntc dft2lnt ${YAML_CPP_LIB})
if (WIN32)
	target_link_libraries(dft2lntc pathcch)
endif()

## POST_BUILD: copy the executable to the bin/ folder
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

## Installation
install (TARGETS dft2lntc DESTINATION bin)

# Generate compile time information
set(GENINFOFILE ${CMAKE_CURRENT_BINARY_DIR}/compiletime.h)
get_property(gendepSOURCES TARGET dft2lntc PROPERTY SOURCES)
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
		-DFALLBACK_VERSION=${dft2lntc_VERSION}
		-DNO_GIT=${NO_GIT}
		-P ${CMAKE_CURRENT_SOURCE_DIR}/../compiletime.cmake
)
