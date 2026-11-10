cmake_minimum_required(VERSION 3.28)

include(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake"
    NO_POLICY_SCOPE
)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" OFF)
option(ENABLE_QT "Use Qt functionality" OFF)
option(ENABLE_TESTING "Build tests (requires OBS dev headers)" OFF)

include(compilerconfig)
include(defaults)
include(helpers)

set(CORE_SOURCES
    src/plugin-main.c
    src/lens-core.c
    src/lens-ui.c
    src/obs-utils.c
)

set(STRATEGY_SOURCES
    src/strategies/strategies.c
    src/strategies/strategy-vidicon.c
    src/strategies/strategy-starburst.c
    src/strategies/strategy-leak.c
    src/strategies/strategy-handheld.c
    src/strategies/strategy-bokeh.c
    src/strategies/strategy-particles.c
    src/strategies/strategy-orton.c
    src/strategies/strategy-grit.c
    src/strategies/strategy-mirage.c
    src/strategies/strategy-spot.c
    src/strategies/strategy-bloom.c
    src/strategies/strategy-pinhole.c
    src/strategies/strategy-polaroid.c
    src/strategies/strategy-daguerreotype.c
    src/strategies/strategy-technicolor.c
    src/strategies/strategy-wet-lens.c
    src/strategies/strategy-phone-wet.c)
add_library(${CMAKE_PROJECT_NAME} MODULE)
# The user-facing OBS module reports the buildspec version, not the dev fallback.
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE PLUGIN_VERSION="${PROJECT_VERSION}")

find_package(libobs CONFIG REQUIRED)
if(NOT DEFINED libobs_VERSION OR libobs_VERSION VERSION_LESS "32.1.0")
    message(FATAL_ERROR "emuLensFX requires libobs 32.1.0 or newer; found ${libobs_VERSION}")
endif()
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

if(ENABLE_FRONTEND_API)
    find_package(obs-frontend-api REQUIRED)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
    find_package(Qt6 REQUIRED COMPONENTS Widgets Core)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
    target_compile_options(
        ${CMAKE_PROJECT_NAME}
        PRIVATE
            $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header
            -Wno-comma>
    )
    set_target_properties(
        ${CMAKE_PROJECT_NAME}
        PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
    )
endif()

# Metal framework detection for Apple Silicon
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    find_library(METAL_FRAMEWORK Metal)
    if(METAL_FRAMEWORK)
        message(
            STATUS
            "Metal framework detected - will enable GPU optimizations"
        )
        target_compile_definitions(
            ${CMAKE_PROJECT_NAME}
            PRIVATE OBS_METAL_AVAILABLE
        )
    endif()
endif()

# ARM64 detection for NEON optimizations
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64|ARM64")
    message(STATUS "Building for ARM64 - will enable NEON optimizations")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE USE_ARM_NEON)
endif()

target_include_directories(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/strategies
)

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE ${CORE_SOURCES} ${STRATEGY_SOURCES}
)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME "emuLensFX")

if(ENABLE_TESTING)
    message(STATUS "Tests enabled - configuring test project")
    add_subdirectory(tests)
endif()
