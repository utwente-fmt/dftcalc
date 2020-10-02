if(USECOMMANDLINEGITINFO STREQUAL "YES")
else()
	execute_process(
		COMMAND git rev-parse --verify HEAD
		OUTPUT_VARIABLE git_output OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	execute_process(
		COMMAND git describe HEAD
		OUTPUT_VARIABLE git_descr OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	execute_process(
		COMMAND git diff --exit-code
		OUTPUT_QUIET
		ERROR_QUIET
		RESULT_VARIABLE git_changed
	)
	set(git_version "*.*.*")
	execute_process(
		COMMAND git describe --tags --match=v[0-9]* --abbrev=0
		OUTPUT_VARIABLE git_version OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	execute_process(
		COMMAND git branch
		OUTPUT_VARIABLE git_test OUTPUT_STRIP_TRAILING_WHITESPACE
	)
endif()

if (git_version STREQUAL git_descr)
else()
	set(git_version ${git_version}+)
endif()

string(TIMESTAMP date "%Y-%m-%d %H:%M:%S")

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/compiletime.h
"/**
 * Generated at compile time on ${date}
 */
#define DFT2LNTROOT \"${DFTROOT}\"
#define COMPILETIME_DATE \"${date}\"
#define COMPILETIME_GITREV \"${git_output}\"
#define COMPILETIME_GITCHANGED ${git_changed}
#define COMPILETIME_GITVERSION \"${git_version}\"
")
