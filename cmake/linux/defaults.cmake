# CMake Linux defaults module

include_guard(GLOBAL)

# Set default installation directories
include(GNUInstallDirs)

if(CMAKE_INSTALL_LIBDIR MATCHES "(CMAKE_SYSTEM_PROCESSOR)")
    string(
        REPLACE
        "CMAKE_SYSTEM_PROCESSOR"
        "${CMAKE_SYSTEM_PROCESSOR}"
        CMAKE_INSTALL_LIBDIR
        "${CMAKE_INSTALL_LIBDIR}"
    )
endif()

# Enable find_package targets to become globally available targets
set(CMAKE_FIND_PACKAGE_TARGETS_GLOBAL TRUE)

option(EMULENS_BUILD_OBS_FROM_SOURCE "Build the pinned OBS SDK from source" OFF)

if(EMULENS_BUILD_OBS_FROM_SOURCE)
    include(buildspec)

    # Source mode must actually use the SDK it just built. Preserve an
    # explicitly supplied libobs_DIR (argument or environment) as an override.
    set(_emulens_pinned_libobs_dir "${CMAKE_CURRENT_SOURCE_DIR}/.deps/lib/cmake/libobs")
    if(NOT libobs_DIR AND NOT DEFINED ENV{libobs_DIR})
        if(NOT EXISTS "${_emulens_pinned_libobs_dir}/libobsConfig.cmake")
            message(FATAL_ERROR "Pinned libobs package was not installed at ${_emulens_pinned_libobs_dir}")
        endif()
        set(
            libobs_DIR
            "${_emulens_pinned_libobs_dir}"
            CACHE PATH
            "Path to pinned source-built libobsConfig.cmake"
            FORCE
        )
    endif()
endif()

set(CPACK_PACKAGE_NAME "${CMAKE_PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${CMAKE_PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME
    "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_C_LIBRARY_ARCHITECTURE}"
)

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${PLUGIN_EMAIL}")
set(CPACK_SET_DESTDIR ON)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.25.0 OR NOT CMAKE_CROSSCOMPILING)
    set(CPACK_DEBIAN_DEBUGINFO_PACKAGE ON)
endif()

set(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/release")

set(CPACK_SOURCE_GENERATOR "TXZ")
set(CPACK_SOURCE_IGNORE_FILES
    ".*~$"
    \\.git/
    \\.github/
    \\.gitignore
    \\.ccache/
    # CPack reads the working tree, not Git's ignore rules.
    \\.meristem/
    \\.deps/
    /build/
    /build-test/
    /build_test/
    /tests/build/
    /install/
    /\\.cache/
    /memory/
    /\\.grepai/
    /mcp\\.db$
    /AGENTS\\.md$
    /ACTIVE_WORK\\.md$
    /SESSION-STATE\\.md$
    /SOUL\\.md$
    /USER\\.md$
    /MEMORY\\.md$
    /HEARTBEAT\\.md$
    /\\.env
    \\.pem$
    \\.key$
    build_.*
    cmake/\\.CMakeBuildNumber
    # Match only the generated release directory, not a checkout whose name
    # happens to end in -release (which would otherwise empty the archive).
    /release/
)

set(CPACK_VERBATIM_VARIABLES YES)
set(CPACK_SOURCE_PACKAGE_FILE_NAME
    "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-source"
)
set(CPACK_ARCHIVE_THREADS 0)

include(CPack)

set(_libobs_config_dir "")
if(
    EXISTS
        "/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}/cmake/libobs/libobsConfig.cmake"
)
    set(_libobs_config_dir
        "/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}/cmake/libobs"
    )
elseif(EXISTS "/usr/lib/x86_64-linux-gnu/cmake/libobs/libobsConfig.cmake")
    set(_libobs_config_dir "/usr/lib/x86_64-linux-gnu/cmake/libobs")
endif()

if(
    NOT EMULENS_BUILD_OBS_FROM_SOURCE
    AND _libobs_config_dir
    AND NOT libobs_DIR
    AND NOT DEFINED ENV{libobs_DIR}
)
    set(libobs_DIR
        "${_libobs_config_dir}"
        CACHE PATH
        "Path to libobsConfig.cmake"
    )
endif()

find_package(libobs QUIET)

if(NOT TARGET OBS::libobs)
    find_package(LibObs REQUIRED)
    add_library(OBS::libobs ALIAS libobs)

    if(ENABLE_FRONTEND_API)
        find_path(
            obs-frontend-api_INCLUDE_DIR
            NAMES obs-frontend-api.h
            PATHS /usr/include /usr/local/include
            PATH_SUFFIXES obs
        )

        find_library(
            obs-frontend-api_LIBRARY
            NAMES obs-frontend-api
            PATHS /usr/lib /usr/local/lib
        )

        if(obs-frontend-api_LIBRARY)
            if(NOT TARGET OBS::obs-frontend-api)
                if(IS_ABSOLUTE "${obs-frontend-api_LIBRARY}")
                    add_library(OBS::obs-frontend-api UNKNOWN IMPORTED)
                    set_property(
                        TARGET OBS::obs-frontend-api
                        PROPERTY IMPORTED_LOCATION "${obs-frontend-api_LIBRARY}"
                    )
                else()
                    add_library(OBS::obs-frontend-api INTERFACE IMPORTED)
                    set_property(
                        TARGET OBS::obs-frontend-api
                        PROPERTY IMPORTED_LIBNAME "${obs-frontend-api_LIBRARY}"
                    )
                endif()

                set_target_properties(
                    OBS::obs-frontend-api
                    PROPERTIES
                        INTERFACE_INCLUDE_DIRECTORIES
                            "${obs-frontend-api_INCLUDE_DIR}"
                )
            endif()
        endif()
    endif()

    macro(find_package)
        if(
            NOT "${ARGV0}" STREQUAL libobs
            AND NOT "${ARGV0}" STREQUAL obs-frontend-api
        )
            _find_package(${ARGV})
        endif()
    endmacro()
endif()
