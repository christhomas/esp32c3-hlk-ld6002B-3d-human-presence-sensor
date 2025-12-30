# Explicitly list source files (recommended over GLOB for ESP-IDF)
set(app_sources
    "main.c"
    "hlk_ld6002.c"
    "target_tracker.c"
    "api.c"
    "web_server.c"
    "wifi_manager.c"
)

# Get project root directory (parent of src/)
get_filename_component(PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)

# Detect Python interpreter for build scripts
# ESP-IDF sets IDF_PYTHON, otherwise try Python3_EXECUTABLE from CMake, or find python3
if(NOT PYTHON)
    if(DEFINED ENV{IDF_PYTHON} AND EXISTS "$ENV{IDF_PYTHON}")
        set(PYTHON "$ENV{IDF_PYTHON}")
    elseif(Python3_EXECUTABLE AND EXISTS "${Python3_EXECUTABLE}")
        set(PYTHON "${Python3_EXECUTABLE}")
    else()
        find_program(PYTHON NAMES python3 python)
        if(NOT PYTHON)
            message(FATAL_ERROR "Python interpreter not found. Please ensure python3 is on PATH or set IDF_PYTHON environment variable.")
        endif()
    endif()
endif()

message(STATUS "Using Python interpreter: ${PYTHON}")

# Setup paths for webapp build
set(WEBAPP_SRC_DIR ${CMAKE_CURRENT_LIST_DIR})
set(WEBAPP_HTML ${WEBAPP_SRC_DIR}/webapp.html)
set(WEBAPP_CSS ${WEBAPP_SRC_DIR}/webapp.css)
set(WEBAPP_JS ${WEBAPP_SRC_DIR}/webapp.js)
set(WEBAPP_BUILD_SCRIPT ${PROJECT_DIR}/tools/build_webapp.py)
set(WEBAPP_OUT ${CMAKE_CURRENT_BINARY_DIR}/webapp.min.html)

# Register component with embedded minified file
idf_component_register(
    SRCS ${app_sources}
    EMBED_TXTFILES ${WEBAPP_OUT}
    REQUIRES
        driver
        esp_http_server
        esp_wifi
        nvs_flash
        mdns
        lwip
        json
)

# Build webapp during compilation (runs when source files change)
# Must be AFTER idf_component_register() for ESP-IDF compatibility
add_custom_command(
    OUTPUT ${WEBAPP_OUT}
    COMMAND "${PYTHON}" ${WEBAPP_BUILD_SCRIPT} compile ${WEBAPP_HTML} ${WEBAPP_CSS} ${WEBAPP_JS} ${WEBAPP_OUT}
    DEPENDS ${WEBAPP_HTML} ${WEBAPP_CSS} ${WEBAPP_JS} ${WEBAPP_BUILD_SCRIPT}
    WORKING_DIRECTORY ${WEBAPP_SRC_DIR}
    COMMENT "Compiling webapp (HTML + CSS + JS)"
    VERBATIM
)

# Create a custom target to ensure webapp is built
add_custom_target(webapp_build ALL DEPENDS ${WEBAPP_OUT})

# Add dependency so webapp builds before component
add_dependencies(${COMPONENT_LIB} webapp_build)
