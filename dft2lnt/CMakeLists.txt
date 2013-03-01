## DFT2LNT Library project
project (dft2lnt)
set (dft2lnt_VERSION_MAJOR 1)
set (dft2lnt_VERSION_MINOR 0)
set (dft2lnt_VERSION_PATCH 2)

## Needed for bison and flex
cmake_minimum_required (VERSION 2.8)

## Make sure we have Bison and FLEX available
include(FindBISON)
include(FindFLEX)

## Not sure if needed
add_definitions( -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS )
#add_definitions(-std=c++0x -D_GNU_SOURCE)
add_definitions(-std=gnu++0x -Wall)

## Include source directory and the build directory (for generated files)
include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../yaml-cpp/include
)

## Generate parser and lexer files to the target directory, because they depend on the selected Build (Debug/Release)
## We need to explicitely state that we want a lexer header file
bison_target(par ${CMAKE_CURRENT_SOURCE_DIR}/parser.y.ypp ${CMAKE_CURRENT_BINARY_DIR}/parser.y.cpp COMPILE_FLAGS "${BISON_CFLAGS}")
flex_target(lex ${CMAKE_CURRENT_SOURCE_DIR}/lexer.l.lpp ${CMAKE_CURRENT_BINARY_DIR}/lexer.l.cpp COMPILE_FLAGS --header-file=\"${CMAKE_CURRENT_BINARY_DIR}/lexer.l.h\")
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
	DFTreeBCGNodeBuilder.cpp
	DFTree.cpp
	DFTreeValidator.cpp
	DFTreePrinter.cpp
	DFTreeSVLAndLNTBuilder.cpp
	DFTreeEXPBuilder.cpp
	DFTCalculationResult.cpp
	files.cpp
	FileWriter.cpp
	ConsoleWriter.cpp
	compiler.cpp
	MessageFormatter.cpp
	Shell.cpp
	FileSystem.cpp
	System.cpp
	imca.cpp
	mrmc.cpp
	CADP.cpp
	${BISON_par_OUTPUTS} ${FLEX_lex_OUTPUTS}
)

target_link_libraries(dft2lnt rt)

# For clock_gettime() (to measure elapsed time), we need this
if(UNIX AND NOT APPLE AND NOT WIN32)
	target_link_libraries(dft2lnt rt)
endif()

## Installation
install (TARGETS dft2lnt DESTINATION lib)
install (FILES dft2lnt.h DESTINATION include/dft2lnt)
