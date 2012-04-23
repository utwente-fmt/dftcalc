## The DFT2LNT Compiler project
project (dft2lntc)
set (dft2lntc_VERSION_MAJOR 1)
set (dft2lntc_VERSION_MINOR 0)
set (dft2lntc_VERSION_PATCH 2)

## Not sure if needed
cmake_minimum_required (VERSION 2.6)

## Not sure if needed
add_definitions( -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS )
#add_definitions(-std=c++0x -D_GNU_SOURCE)
add_definitions(-std=gnu++0x)

## Include source directory, the build directory (for generated files) and the dft2lnt library path
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../dft2lnt)

## Link directories
#link_directories(${CMAKE_CURRENT_BINARY_DIR}/../dft2lnt)

## Specify the executable and its sources
add_executable(dft2lntc
	dft2lntc.cpp
	${CMAKE_CURRENT_BINARY_DIR}/compiletime.h
)

## Depends on:
##   - dft2lnt (libdft2lnt.a)
add_dependencies(dft2lntc dft2lnt yaml-cpp)


## Link with:
##   - libdft2lnt.a
target_link_libraries(dft2lntc ${CMAKE_CURRENT_BINARY_DIR}/../dft2lnt/libdft2lnt.a ${CMAKE_CURRENT_BINARY_DIR}/../yaml-cpp/libyaml-cpp.a)

## POST_BUILD: copy the executable to the bin/ folder
GET_TARGET_PROPERTY(BIN_LOCATION   dft2lntc LOCATION)
SET(NEW_LIB_NAME ${CMAKE_CURRENT_BINARY_DIR}/../bin/dft2lntc${CMAKE_EXECUTABLE_SUFFIX})
ADD_CUSTOM_COMMAND(
  TARGET dft2lntc
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${BIN_LOCATION} ${NEW_LIB_NAME}
)

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
				-P ${CMAKE_CURRENT_SOURCE_DIR}/../compiletime.cmake
				../..
	)