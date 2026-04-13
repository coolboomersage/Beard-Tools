cmake_minimum_required(VERSION 3.10)

project(BeardLoganalyzer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR})

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR})

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR})


# ---------------------- libcurl -----------------------------------
set(BUILD_CURL_EXE     OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS  ON  CACHE BOOL "" FORCE)
set(CURL_DISABLE_TESTS ON  CACHE BOOL "" FORCE)
set(CURL_USE_LIBPSL    OFF CACHE BOOL "" FORCE)
set(CURL_USE_SCHANNEL  ON  CACHE BOOL "" FORCE)

add_subdirectory(
    ${CMAKE_SOURCE_DIR}/external/curl-8.19.0
    ${CMAKE_BINARY_DIR}/curl-build
)

#---------------------- imgui ---------------------------------------
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui-1.92.7)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# ----------------------- exe file -------------------------------------
add_executable(BeardTools main.cpp ${IMGUI_SOURCES})

target_include_directories(BeardTools PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${CMAKE_SOURCE_DIR}/external/glfw-3.4/include
    ${CMAKE_SOURCE_DIR}/external
    ${CMAKE_SOURCE_DIR}/external/curl-8.19.0/include
)

target_link_libraries(BeardTools PRIVATE
    ${CMAKE_SOURCE_DIR}/external/glfw-3.4/build/src/debug/glfw3.lib
    libcurl
    opengl32
)

set_target_properties(BeardTools PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}
)

# ----------------------- copy libcurl dll from source to build -------------------------
add_custom_command(TARGET BeardTools POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:libcurl>
        $<TARGET_FILE_DIR:BeardTools>/libcurl.dll
    COMMENT "Copying libcurl.dll -> ${CMAKE_BINARY_DIR}"
)

#---------------- download cacert.pem into build -----------------
set(CACERT_PATH ${CMAKE_BINARY_DIR}/cacert.pem)
if(NOT EXISTS ${CACERT_PATH})
    message(STATUS "Downloading cacert.pem...")
    file(DOWNLOAD
        "https://curl.se/ca/cacert.pem"
        ${CACERT_PATH}
        SHOW_PROGRESS
        STATUS DOWNLOAD_STATUS
    )
    list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
    if(NOT STATUS_CODE EQUAL 0)
        message(WARNING "Failed to download cacert.pem - SSL verification may fail at runtime")
    endif()
endif()