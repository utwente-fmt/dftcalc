## DFT2LNT Library project
project (dft2lnt)
set (dft2lnt_VERSION_MAJOR 0)
set (dft2lnt_VERSION_MINOR 1)

## Not sure if needed
cmake_minimum_required (VERSION 2.6)

## Make sure we have Bison and FLEX available
include(FindBISON)
include(FindFLEX)

## Not sure if needed
add_definitions( -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS )
#add_definitions(-std=c++0x -D_GNU_SOURCE)
add_definitions(-std=gnu++0x)

## Include source directory and the build directory (for generated files)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

## Generate parser and lexer files to the target directory, because they depend on the selected Build (Debug/Release)
## We need to explicitely state that we want a lexer header file
bison_target(par parser.y.ypp ${CMAKE_CURRENT_BINARY_DIR}/parser.y.cpp)
flex_target(lex lexer.l.lpp ${CMAKE_CURRENT_BINARY_DIR}/lexer.l.cpp COMPILE_FLAGS --header-file=\"${CMAKE_CURRENT_BINARY_DIR}/lexer.l.h\")
ADD_FLEX_BISON_DEPENDENCY(lex par)

## By default the header is not cleaned up (nor built)
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/lexer.l.h)

## Specify the library and its sources
add_library(dft2lnt
	dft2lnt.cpp
	dft_parser.cpp
	dft_commands.cpp
	dftnodes/BasicEvent.cpp
	DFTree.cpp
	DFTreeValidator.cpp
	DFTreePrinter.cpp
	DFTreeSVLAndLNTBuilder.cpp
	${BISON_par_OUTPUTS} ${FLEX_lex_OUTPUTS}
)

## Installation
install (TARGETS dft2lnt DESTINATION bin)
install (FILES compiler.h DESTINATION include)
