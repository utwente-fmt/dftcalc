## DFT2LNT Library project
project (dftmisc)

## Needed for bison and flex
cmake_minimum_required (VERSION 2.8)

set(DFTROOT "/opt/dft2lntroot" CACHE PATH "")

message(STATUS "NOTE: When installed, dft2lnt will be installed in ${DFTROOT}")
message(STATUS "      You can change this behaviour by running cmake with:")
message(STATUS "         -DDFTROOT=/path/to/dft2lntroot")

set(CMAKE_INSTALL_PREFIX ${DFTROOT} CACHE INTERNAL "" FORCE)

## Installation
FILE(GLOB lntfiles "${CMAKE_CURRENT_SOURCE_DIR}/lntnodes/*.lnt")
FILE(GLOB dftfiles "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.dft")
FILE(GLOB testfiles "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.test")
install (DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lntnodes"
	DESTINATION share/dft2lnt
	FILE_PERMISSIONS OWNER_READ
	                 GROUP_READ
	                 WORLD_READ
	
	DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
	                      GROUP_READ GROUP_WRITE GROUP_EXECUTE
	                      WORLD_READ WORLD_WRITE WORLD_EXECUTE
)
#install (FILES ${lntfiles}
#	DESTINATION share/dft2lnt/lntnodes
#	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
#	            GROUP_READ GROUP_WRITE GROUP_EXECUTE
#	            WORLD_READ WORLD_WRITE WORLD_EXECUTE
#)
install (DIRECTORY
	DESTINATION share/dft2lnt/bcgnodes/
	DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
	                      GROUP_READ GROUP_WRITE GROUP_EXECUTE
	                      WORLD_READ WORLD_WRITE WORLD_EXECUTE
)
install (FILES ${dftfiles}
	DESTINATION share/dft2lnt/tests
	PERMISSIONS OWNER_READ
	            GROUP_READ
	            WORLD_READ
)
install (FILES ${testfiles}
	DESTINATION share/dft2lnt/tests
	PERMISSIONS OWNER_READ
	            GROUP_READ
	            WORLD_READ
)
