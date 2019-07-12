## The DFT2LNT Compiler project
project (dft2lntc)
set (dft2lntc_VERSION_MAJOR 1)
set (dft2lntc_VERSION_MINOR 3)
set (dft2lntc_VERSION_PATCH 0)

## Not sure if needed
cmake_minimum_required (VERSION 2.6)

add_definitions(-std=c++11)
if (DEFINED ENV{CADP})
	add_definitions(-DHAVE_CADP)
endif()

## Include source directory, the build directory (for generated files) and the dft2lnt library path
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../dft2lnt)

## Link directories
#link_directories(${CMAKE_CURRENT_BINARY_DIR}/../dft2lnt)

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
	${ADDITIONAL_SOURCES}
)

## Depends on:
##   - dft2lnt (libdft2lnt.a)
add_dependencies(dft2lntc dft2lnt)


## Link with:
##   - libdft2lnt.a
target_link_libraries(dft2lntc ${CMAKE_CURRENT_BINARY_DIR}/../dft2lnt/libdft2lnt.a yaml-cpp)

## POST_BUILD: copy the executable to the bin/ folder
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

## Installation
install (TARGETS dft2lntc DESTINATION bin)

# Generate compile time information
	set(GENINFOFILE ${CMAKE_CURRENT_BINARY_DIR}/compiletime.h)
	get_property(gendepSOURCES TARGET dft2lntc PROPERTY SOURCES)
	LIST(REMOVE_ITEM gendepSOURCES ${GENINFOFILE})
	add_custom_command(
		OUTPUT ${GENINFOFILE}
	#	TARGET dft2lntc
	#	PRE_BUILD
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../compiletime.cmake ${gendepSOURCES}
		COMMAND ${CMAKE_COMMAND} -DCMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
				-DCMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
				-DDFTROOT=${DFTROOT}
				-DUSECOMMANDLINEGITINFO=${USECOMMANDLINEGITINFO} -Dgit_output=${git_output} -Dgit_changed=${git_changed} -Dgit_version=${git_version} -Dgit_test=${git_test}
				-P ${CMAKE_CURRENT_SOURCE_DIR}/../compiletime.cmake
				../..
	)
