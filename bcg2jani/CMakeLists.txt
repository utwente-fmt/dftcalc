project (bcg2jani)
set (bcg2jani_VERSION_MAJOR 1)
set (bcg2jani_VERSION_MINOR 0)
set (bcg2jani_VERSION_PATCH 0)

add_definitions(-std=c99)

include_directories($ENV{CADP}/incl)

## Specify the executable and its sources
add_executable(bcg2jani bcg2jani.c)

## POST_BUILD: copy the executable to the bin/ folder
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

## Installation
install (TARGETS bcg2jani DESTINATION bin)

LINK_DIRECTORIES($ENV{CADP})
find_library(BCG_LIB BCG)
find_library(BCG_IO_LIB BCG_IO)
target_link_libraries(bcg2jani "${BCG_LIB}" "${BCG_IO_LIB}")
