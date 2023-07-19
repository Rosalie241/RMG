set(GIT_BRANCH "unknown")
set(GIT_COMMIT_DATE "unknown")
set(GIT_COMMIT_HASH "unknown")
set(GIT_TAG "unknown")

find_package(Git)
if(GIT_FOUND AND EXISTS "${SOURCE_DIR}/.git/")
	execute_process(
		COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
		WORKING_DIRECTORY ${SOURCE_DIR}
		RESULT_VARIABLE GIT_BRANCH_RESULT
		OUTPUT_VARIABLE GIT_BRANCH
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	if(NOT ${GIT_BRANCH_RESULT} EQUAL 0)
		message(WARNING "git rev-parse failed, unknown branch.")
	endif()

	execute_process(
		COMMAND ${GIT_EXECUTABLE} show -s --format=%cs
		WORKING_DIRECTORY ${SOURCE_DIR}
		RESULT_VARIABLE GIT_COMMIT_DATE_RESULT
		OUTPUT_VARIABLE GIT_COMMIT_DATE
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	if(NOT ${GIT_COMMIT_DATE_RESULT} EQUAL 0)
		message(WARNING "git show failed, unknown commit date.")
	endif()

	execute_process(
		COMMAND ${GIT_EXECUTABLE} show -s --format=%h
		WORKING_DIRECTORY ${SOURCE_DIR}
		RESULT_VARIABLE GIT_COMMIT_HASH_RESULT
		OUTPUT_VARIABLE GIT_COMMIT_HASH
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	if(NOT ${GIT_COMMIT_HASH_RESULT} EQUAL 0)
		message(WARNING "git show failed, unknown commit hash.")
	endif()

	execute_process(
		COMMAND ${GIT_EXECUTABLE} describe --dirty --always --tags
		WORKING_DIRECTORY ${SOURCE_DIR}
		RESULT_VARIABLE GIT_TAG_RESULT
		OUTPUT_VARIABLE GIT_TAG
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	if(NOT ${GIT_TAG_RESULT} EQUAL 0)
		message(WARNING "git describe failed, unknown tag.")
	endif()
else()
	message(WARNING "git not found or not a git repo.")
endif()

configure_file(${PATH_VERSION}.in ${PATH_VERSION} @ONLY)
