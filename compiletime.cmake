execute_process(
	COMMAND date "+%Y-%m-%d %H:%M:%S"
	OUTPUT_VARIABLE date_output OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
	COMMAND git rev-parse --verify HEAD
	OUTPUT_VARIABLE git_output OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
	COMMAND git diff --exit-code > /dev/null
	RESULT_VARIABLE git_changed OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(git_version "*.*.*")
execute_process(
	COMMAND git log --tags=*[.]*[.]* --pretty=format:%d -n1
	COMMAND grep -m1 -oe "[[:digit:]][.][[:digit:]][.][[:digit:]]"
	OUTPUT_VARIABLE git_version OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
	COMMAND git branch
	OUTPUT_VARIABLE git_test OUTPUT_STRIP_TRAILING_WHITESPACE
)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/compiletime.h
"/**
 * Generated at compile time on ${date_output}
 */
#define COMPILETIME_DATE \"${date_output}\"
#define COMPILETIME_GITREV \"${git_output}\"
#define COMPILETIME_GITCHANGED \"${git_changed}\"
#define COMPILETIME_GITVERSION \"${git_version}\"
")
