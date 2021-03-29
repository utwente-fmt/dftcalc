project(dftcalc-suite)

cmake_minimum_required(VERSION 3.0)

if(CMAKE_BUILD_TYPE STREQUAL "")
	message("Defaulting to Release configuration")
	set(CMAKE_BUILD_TYPE "Release")
endif()

message("Building configuration: ${CMAKE_BUILD_TYPE}")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_definitions(-DDEBUG=1 -DYYDEBUG=1)
	set (BISON_CFLAGS "${BISON_CFLAGS} --verbose")
endif()

set(DFTROOT "/opt/dft2lntroot" CACHE PATH "")

message(STATUS "NOTE: When installed, dft2lnt will be installed in ${DFTROOT}")
message(STATUS "      You can change this behaviour by running cmake with:")
message(STATUS "         -DDFTROOT=/path/to/dft2lntroot")

add_subdirectory(dft2lnt)
add_subdirectory(dft2lntc)
add_subdirectory(dftcalc)
add_subdirectory(dfttest)
if(DEFINED ENV{CADP})
	add_subdirectory(bcgconv)
endif()
add_subdirectory(share)
