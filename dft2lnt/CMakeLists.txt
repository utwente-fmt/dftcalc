## DFT2LNT Library project
project (dft2lnt)
set (dft2lnt_VERSION_MAJOR 1)
set (dft2lnt_VERSION_MINOR 3)
set (dft2lnt_VERSION_PATCH 4)

## Needed for bison and flex
cmake_minimum_required (VERSION 2.8)

## Make sure we have Bison and FLEX available
include(FindBISON)
include(FindFLEX)

add_definitions(-std=c++17)

## Include source directory and the build directory (for generated files)
include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
)

if(DEFINED ENV{CADP})
	set(CADP_SOURCES CADP.cpp)
else()
	set(CADP_SOURCES)
endif()

## Generate parser and lexer files to the target directory, because they depend on the selected Build (Debug/Release)
## We need to explicitely state that we want a lexer header file
bison_target(par ${CMAKE_CURRENT_SOURCE_DIR}/parser.y.ypp ${CMAKE_CURRENT_BINARY_DIR}/parser.y.cpp COMPILE_FLAGS "${BISON_CFLAGS}")
flex_target(lex ${CMAKE_CURRENT_SOURCE_DIR}/lexer.l.lpp ${CMAKE_CURRENT_BINARY_DIR}/lexer.l.cpp COMPILE_FLAGS "--header-file=${CMAKE_CURRENT_BINARY_DIR}/lexer.l.h")
ADD_FLEX_BISON_DEPENDENCY(lex par)

## By default the header is not cleaned up (nor built)
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/lexer.l.h)

## Specify the library and its sources
add_library(dft2lnt
	dft2lnt.cpp
	dft_ast.cpp
	dft_parser.cpp
	dftnodes/BasicEvent.cpp
	dftnodes/Node.cpp
	DFTree.cpp
	DFTreeValidator.cpp
	DFTreePrinter.cpp
	DFTCalculationResult.cpp
	files.cpp
	FileWriter.cpp
	ConsoleWriter.cpp
	compiler.cpp
	MessageFormatter.cpp
	Shell.cpp
	FileSystem.cpp
	System.cpp
	${CADP_SOURCES}
	${BISON_par_OUTPUTS} ${FLEX_lex_OUTPUTS}
)

target_link_libraries(dft2lnt rt)

## Installation
install (TARGETS dft2lnt DESTINATION lib)
install (FILES dft2lnt.h DESTINATION include/dft2lnt)
