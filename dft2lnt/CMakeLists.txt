## DFT2LNT Library project
project (dft2lnt)
set (dft2lnt_VERSION_MAJOR 1)
set (dft2lnt_VERSION_MINOR 4)
set (dft2lnt_VERSION_PATCH 1)

## Needed for bison and flex
cmake_minimum_required (VERSION 3.0)

## Make sure we have Bison and FLEX available
find_package(BISON 3.3)
find_package(FLEX 2.6.3)

if (MSVC)
	add_compile_options(/std:c++17)
else()
	add_compile_options(-std=c++17)
endif()

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
if (WIN32)
	flex_target(lex ${CMAKE_CURRENT_SOURCE_DIR}/lexer.l.lpp ${CMAKE_CURRENT_BINARY_DIR}/lexer.l.cpp COMPILE_FLAGS --wincompat DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/lexer.l.h)
else()
	flex_target(lex ${CMAKE_CURRENT_SOURCE_DIR}/lexer.l.lpp ${CMAKE_CURRENT_BINARY_DIR}/lexer.l.cpp DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/lexer.l.h)
endif()
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

# Find yaml-cpp
find_path(YAML_INCLUDE_PATH "yaml-cpp/yaml.h")

if(UNIX AND NOT APPLE AND NOT WIN32)
	target_link_libraries(dft2lnt rt)
endif()

target_include_directories(dft2lnt PRIVATE ${YAML_INCLUDE_PATH})

if (NOT DEFINED LIB_INSTALL_DIR)
	if (DEFINED CMAKE_INSTALL_LIBDIR)
		set(LIB_INSTALL_DIR "lib")
	else()
		set(LIB_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR})
	endif()
endif()

## Installation
install (TARGETS dft2lnt DESTINATION ${LIB_INSTALL_DIR})
install (FILES dft2lnt.h DESTINATION include/dft2lnt)
