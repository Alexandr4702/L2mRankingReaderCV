if(NOT DEFINED EXECUTABLE OR NOT EXISTS "${EXECUTABLE}")
    message(FATAL_ERROR "Runtime dependency scan requires an existing executable")
endif()

if(POLICY CMP0207)
    cmake_policy(SET CMP0207 NEW)
endif()

if(NOT DEFINED DESTINATION)
    message(FATAL_ERROR "Runtime dependency destination is not set")
endif()

get_filename_component(COMPILER_DIRECTORY "${COMPILER_DIRECTORY}" DIRECTORY)

file(GET_RUNTIME_DEPENDENCIES
    EXECUTABLES "${EXECUTABLE}"
    RESOLVED_DEPENDENCIES_VAR resolved_dependencies
    UNRESOLVED_DEPENDENCIES_VAR unresolved_dependencies
    CONFLICTING_DEPENDENCIES_PREFIX dependency_conflicts
    DIRECTORIES "${COMPILER_DIRECTORY}"
    PRE_EXCLUDE_REGEXES
        "api-ms-win-.*"
        "ext-ms-win-.*"
    POST_EXCLUDE_REGEXES
        "^[A-Za-z]:/[Ww][Ii][Nn][Dd][Oo][Ww][Ss]/[Ss][Yy][Ss][Tt][Ee][Mm]32/.*"
)

# A parallel build may find the same DLL both in the compiler directory and
# already copied next to another executable. The existing file is usable.
if(dependency_conflicts_FILENAMES)
    foreach(filename IN LISTS dependency_conflicts_FILENAMES)
        if(NOT EXISTS "${DESTINATION}/${filename}")
            message(FATAL_ERROR "Conflicting runtime dependency was not previously bundled: ${filename}")
        endif()
    endforeach()
endif()

foreach(dependency IN LISTS resolved_dependencies)
    file(COPY "${dependency}" DESTINATION "${DESTINATION}")
endforeach()

if(unresolved_dependencies)
    list(JOIN unresolved_dependencies ", " unresolved_list)
    message(WARNING "Unresolved runtime dependencies for ${EXECUTABLE}: ${unresolved_list}")
endif()
