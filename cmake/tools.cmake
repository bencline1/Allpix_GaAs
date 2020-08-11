# Retrieve the allpix version string from git describe
FUNCTION(get_version project_version)
    # Check if this is a source tarball build
    IF(NOT IS_DIRECTORY ${CMAKE_SOURCE_DIR}/.git)
        SET(source_package 1)
    ENDIF(NOT IS_DIRECTORY ${CMAKE_SOURCE_DIR}/.git)

    # Set package version
    IF(NOT source_package)
        SET(tag_found FALSE)

        # Get the version from last git tag plus number of additional commits:
        FIND_PACKAGE(Git QUIET)
        IF(GIT_FOUND)
            EXEC_PROGRAM(
                git ${CMAKE_CURRENT_SOURCE_DIR}
                ARGS describe --tags HEAD
                OUTPUT_VARIABLE ${project_version})
            STRING(REGEX REPLACE "^release-" "" ${project_version} ${${project_version}})
            STRING(REGEX REPLACE "([v0-9.]+)-([0-9]+)-([A-Za-z0-9]+)" "\\1+\\2^\\3" ${project_version} ${${project_version}})
            EXEC_PROGRAM(
                git ARGS
                status --porcelain ${CMAKE_CURRENT_SOURCE_DIR}
                OUTPUT_VARIABLE PROJECT_STATUS)
            IF(PROJECT_STATUS STREQUAL "")
                MESSAGE(STATUS "Git project directory is clean.")
            ELSE(PROJECT_STATUS STREQUAL "")
                MESSAGE(STATUS "Git project directory is dirty:\n ${PROJECT_STATUS}.")
            ENDIF(PROJECT_STATUS STREQUAL "")

            # Check if commit flag has been set by the CI:
            IF(DEFINED ENV{CI_COMMIT_TAG})
                MESSAGE(STATUS "Found CI tag flag, building tagged version")
                SET(tag_found TRUE)
            ENDIF()
        ELSE(GIT_FOUND)
            MESSAGE(STATUS "Git repository present, but could not find git executable.")
        ENDIF(GIT_FOUND)
    ELSE(NOT source_package)
        # If we don't have git we can not really do anything
        MESSAGE(STATUS "Source tarball build - no repository present.")
        SET(tag_found TRUE)
    ENDIF(NOT source_package)

    # Set the project version in the parent scope
    SET(TAG_FOUND
        ${tag_found}
        PARENT_SCOPE)

    # Set the project version in the parent scope
    SET(${project_version}
        ${${project_version}}
        PARENT_SCOPE)
ENDFUNCTION()

# Add runtime dependency requirement to the list for generating a bash source file
FUNCTION(add_runtime_dep runtime_name)
    GET_FILENAME_COMPONENT(THISPROG ${runtime_name} PROGRAM)
    LIST(APPEND _ALLPIX_RUNTIME_DEPS ${THISPROG})
    LIST(REMOVE_DUPLICATES _ALLPIX_RUNTIME_DEPS)
    SET(_ALLPIX_RUNTIME_DEPS
        ${_ALLPIX_RUNTIME_DEPS}
        CACHE INTERNAL "ALLPIX_RUNTIME_DEPS")
ENDFUNCTION()

# Add runtime-library requirement to the list for generating a bash source file
FUNCTION(add_runtime_lib runtime_name)
    FOREACH(name ${runtime_name})
        GET_FILENAME_COMPONENT(THISLIB ${name} DIRECTORY)
        LIST(APPEND _ALLPIX_RUNTIME_LIBS ${THISLIB})
        LIST(REMOVE_DUPLICATES _ALLPIX_RUNTIME_LIBS)
        SET(_ALLPIX_RUNTIME_LIBS
            ${_ALLPIX_RUNTIME_LIBS}
            CACHE INTERNAL "ALLPIX_RUNTIME_LIBS")
    ENDFOREACH()
ENDFUNCTION()

FUNCTION(ADD_TOOL_TEST)
    SET(oneValueArgs NAME EXECUTABLE OUTPUT PASS_REGULAR_EXPRESSION)
    SET(multiValueArgs ARGUMENTS)
    CMAKE_PARSE_ARGUMENTS(ADD_TOOL_TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Run the tool
    ADD_TEST(NAME tools/${ADD_TOOL_TEST_NAME}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_INSTALL_PREFIX}/bin/${ADD_TOOL_TEST_EXECUTABLE} ${ADD_TOOL_TEST_ARGUMENTS}
    )

    # Check the tool output field
    ADD_TEST(NAME tools/check_${ADD_TOOL_TEST_NAME}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_INSTALL_PREFIX}/bin/apf_dump --values 10 ${CMAKE_BINARY_DIR}/${ADD_TOOL_TEST_OUTPUT}
    )
    SET_TESTS_PROPERTIES(tools/check_${ADD_TOOL_TEST_NAME} PROPERTIES DEPENDS "${ADD_TOOL_TEST_NAME}")
    SET_TESTS_PROPERTIES(tools/check_${ADD_TOOL_TEST_NAME} PROPERTIES PASS_REGULAR_EXPRESSION "${ADD_TOOL_TEST_PASS_REGULAR_EXPRESSION}")
ENDFUNCTION()
