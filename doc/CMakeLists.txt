#############################################
# Doxygen target to generate API reference  #
#############################################

FIND_PACKAGE(Doxygen)

IF(DOXYGEN_FOUND)
    CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/doc/reference/Doxyfile.in ${CMAKE_BINARY_DIR}/reference/Doxyfile @ONLY)
    ADD_CUSTOM_TARGET(reference
        ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/reference/Doxyfile
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/reference
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
        )
ENDIF(DOXYGEN_FOUND)

#####################################
# Prepare MarkDown files for MkDocs #
#####################################


# Loop through all modules and fetch their README
SET(module_dir ${CMAKE_SOURCE_DIR}/src/modules)
FILE(GLOB subdirs RELATIVE ${module_dir} ${module_dir}/*)
FOREACH(subdir ${subdirs})
    IF(EXISTS ${module_dir}/${subdir}/README.md AND NOT ${subdir} STREQUAL "Dummy")
        CONFIGURE_FILE(${module_dir}/${subdir}/README.md ${CMAKE_BINARY_DIR}/mkdocs/modules/${subdir}.md)
        FILE(READ ${CMAKE_BINARY_DIR}/mkdocs/modules/${subdir}.md markdown_file)
        # Chang GitLab-flavord markdown syntax to MkDocs formula style:
        string(REGEX REPLACE "(\n\\$` *| *`\\$)+" "\$\$" markdown_file "${markdown_file}")
        string(REGEX REPLACE "(\\$` *| *`\\$)+" "\$" markdown_file "${markdown_file}")
        # Get Title:
        string(REGEX MATCH "^# ([A-Za-z0-9 ]+)\n" _ ${markdown_file})
        # Add Markdown header:
        FILE(WRITE ${CMAKE_BINARY_DIR}/mkdocs/modules/${subdir}.md "---\ntemplate: overrides/main.html\ntitle: \"${CMAKE_MATCH_1}\"\n---\n")
        # Write out updated file
        FILE(APPEND ${CMAKE_BINARY_DIR}/mkdocs/modules/${subdir}.md "${markdown_file}")
        SET(module_md_files ${module_md_files} ${CMAKE_BINARY_DIR}/mkdocs/modules/${subdir}.md)
    ENDIF()
ENDFOREACH()

# Loop through all examples and fetch their README
SET(example_dir ${CMAKE_SOURCE_DIR}/examples)
FILE(GLOB subdirs RELATIVE ${example_dir} ${example_dir}/*)
FOREACH(subdir ${subdirs})
    IF(EXISTS ${example_dir}/${subdir}/README.md)
        CONFIGURE_FILE(${example_dir}/${subdir}/README.md ${CMAKE_BINARY_DIR}/mkdocs/examples/${subdir}.md)
        FILE(READ ${CMAKE_BINARY_DIR}/mkdocs/examples/${subdir}.md markdown_file)
        # Chang GitLab-flavord markdown syntax to MkDocs formula style:
        string(REGEX REPLACE "(\n\\$` *| *`\\$)+" "\$\$" markdown_file "${markdown_file}")
        string(REGEX REPLACE "(\\$` *| *`\\$)+" "\$" markdown_file "${markdown_file}")
        # Get Title:
        string(REGEX MATCH "^# ([A-Za-z0-9 ]+)\n" _ ${markdown_file})
        # Add Markdown header:
        FILE(WRITE ${CMAKE_BINARY_DIR}/mkdocs/examples/${subdir}.md "---\ntemplate: overrides/main.html\ntitle: \"${CMAKE_MATCH_1}\"\n---\n")
        # Write out updated file
        FILE(APPEND ${CMAKE_BINARY_DIR}/mkdocs/examples/${subdir}.md "${markdown_file}")
        SET(example_md_files ${example_md_files} ${CMAKE_BINARY_DIR}/usermanual/examples/${subdir}.md)
    ENDIF()
ENDFOREACH()
