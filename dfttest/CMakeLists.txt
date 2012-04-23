## The DFT2LNT Compiler project
project (dfttest)
set (dfttest_VERSION_MAJOR 1)
set (dfttest_VERSION_MINOR 0)
set (dfttest_VERSION_PATCH 2)

## Not sure if needed
cmake_minimum_required (VERSION 2.6)

## Not sure if needed
add_definitions( -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS )
#add_definitions(-std=c++0x -D_GNU_SOURCE)
add_definitions(-std=gnu++0x)

## Include source directory, the build directory (for generated files) and the dft2lnt library path
include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../dft2lnt
	${CMAKE_CURRENT_SOURCE_DIR}/../dftcalc
	${CMAKE_CURRENT_SOURCE_DIR}/../yaml-cpp/include
	${CMAKE_CURRENT_SOURCE_DIR}/../pcre
	${CMAKE_CURRENT_BINARY_DIR}/../pcre/
)

## Link directories
#link_directories(${CMAKE_CURRENT_BINARY_DIR}/../dft2lnt)

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
add_dependencies(dfttest dft2lntc yaml-cpp)

## Link with:
##   - libdft2lnt.a
target_link_libraries(dfttest
	${CMAKE_CURRENT_BINARY_DIR}/../dft2lnt/libdft2lnt.a
	${CMAKE_CURRENT_BINARY_DIR}/../yaml-cpp/libyaml-cpp.a
#	${CMAKE_CURRENT_BINARY_DIR}/../pcre/libpcrecpp.a
	)

if(UNIX AND NOT APPLE AND NOT WIN32)
	target_link_libraries(dfttest rt)
endif()

## POST_BUILD: copy the executable to the bin/ folder
GET_TARGET_PROPERTY(BIN_LOCATION   dfttest LOCATION)
SET(NEW_LIB_NAME ${CMAKE_CURRENT_BINARY_DIR}/../bin/dfttest${CMAKE_EXECUTABLE_SUFFIX})
ADD_CUSTOM_COMMAND(
  TARGET dfttest
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${BIN_LOCATION} ${NEW_LIB_NAME}
)

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
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../compiletime.cmake ${gendepSOURCES}
		COMMAND ${CMAKE_COMMAND} -DCMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
				-DCMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
				-P ${CMAKE_CURRENT_SOURCE_DIR}/../compiletime.cmake
				../..
	)
