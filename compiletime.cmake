if (${NO_GIT})
	set(GIT "GIT-NOTFOUND")
else()
	find_program(GIT git)
endif()

if (${GIT} STREQUAL "GIT-NOTFOUND")
	set(git_output "UNKNOWN")
	set(git_changed 0)
	set(git_version "v${FALLBACK_VERSION}?")
else()
	execute_process(
		COMMAND ${GIT} rev-parse --verify HEAD
		OUTPUT_VARIABLE git_output OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	execute_process(
		COMMAND ${GIT} describe HEAD
		OUTPUT_VARIABLE git_descr OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	execute_process(
		COMMAND ${GIT} diff --exit-code
		OUTPUT_QUIET
		ERROR_QUIET
		RESULT_VARIABLE git_changed
	)
	set(git_version "*.*.*")
	execute_process(
		COMMAND ${GIT} describe --tags --match=v[0-9]* --abbrev=0
		OUTPUT_VARIABLE git_version OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	execute_process(
		COMMAND ${GIT} branch
		OUTPUT_VARIABLE git_test OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	if (git_version STREQUAL git_descr)
	else()
		set(git_version ${git_version}+)
	endif()
endif()

string(TIMESTAMP date "%Y-%m-%d %H:%M:%S")

file(WRITE ${OUTPUT_FILE}
"/**
 * Generated at compile time on ${date}
 */
#define DFT2LNTROOT \"${DFTROOT}\"
#define COMPILETIME_DATE \"${date}\"
#define COMPILETIME_GITREV \"${git_output}\"
#define COMPILETIME_GITCHANGED ${git_changed}
#define COMPILETIME_GITVERSION \"${git_version}\"
")
