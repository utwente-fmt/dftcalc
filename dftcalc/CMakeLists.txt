## The DFT2LNT Compiler project
project (dftcalc)
set (dftcalc_VERSION_MAJOR 1)
set (dftcalc_VERSION_MINOR 1)
set (dftcalc_VERSION_PATCH 0)

## Not sure if needed
cmake_minimum_required (VERSION 2.6)

## Not sure if needed
add_definitions( -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS )
add_definitions(-std=c++11)

## Include source directory, the build directory (for generated files) and the dft2lnt library path
include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../dft2lnt
	${CMAKE_CURRENT_SOURCE_DIR}/../yaml-cpp/include
)

## Link directories
#link_directories(
#	${CMAKE_CURRENT_BINARY_DIR}/../dft2lnt
#)

## Specify the executable and its sources
add_executable(dftcalc
	dftcalc.cpp
	imca.cpp
	storm.cpp
	executor.cpp
	mrmc.cpp
	${CMAKE_CURRENT_BINARY_DIR}/compiletime.h
)

## Depends on:
##   - dft2lnt (libdft2lnt.a)
add_dependencies(dftcalc dft2lntc)


## Link with:
##   - libdft2lnt.a
target_link_libraries(dftcalc ${CMAKE_CURRENT_BINARY_DIR}/../dft2lnt/libdft2lnt.a ${CMAKE_CURRENT_BINARY_DIR}/../yaml-cpp/libyaml-cpp.a)

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
	add_custom_command(
		OUTPUT ${GENINFOFILE}
	#	TARGET dftcalc
	#	PRE_BUILD
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../compiletime.cmake ${gendepSOURCES}
		COMMAND ${CMAKE_COMMAND} -DCMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
				-DCMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
				-DUSECOMMANDLINEGITINFO=${USECOMMANDLINEGITINFO} -Dgit_output=${git_output} -Dgit_changed=${git_changed} -Dgit_version=${git_version} -Dgit_test=${git_test}
				-P ${CMAKE_CURRENT_SOURCE_DIR}/../compiletime.cmake
				../..
	)
message("CMAKE ARGS: ${CMAKE_COMMAND}")
