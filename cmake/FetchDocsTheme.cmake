# Clones the doxygen-awesome-css theme into the docs tree if it is absent, so the
# local `docs` target produces the same modern styling as the published site. The
# checkout is gitignored. Invoked by the `docs` custom target with -P.

if(EXISTS "${THEME_DIR}/doxygen-awesome.css")
    return()
endif()

if(NOT GIT_EXECUTABLE)
    message(WARNING "Git not found; docs will use the default Doxygen theme.")
    return()
endif()

message(STATUS "Fetching doxygen-awesome-css (${THEME_TAG}) into ${THEME_DIR} ...")
execute_process(
    COMMAND "${GIT_EXECUTABLE}" clone --depth 1 --branch "${THEME_TAG}"
            https://github.com/jothepro/doxygen-awesome-css.git "${THEME_DIR}"
    RESULT_VARIABLE clone_result
)
if(NOT clone_result EQUAL 0)
    message(WARNING "Could not fetch doxygen-awesome-css; docs will use the default theme.")
endif()
