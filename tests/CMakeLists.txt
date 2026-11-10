# GPU regression tests load the real module; standalone test-only configuration
# cannot resolve TARGET_FILE:emuLensFX or prove production integration.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(FATAL_ERROR "Configure tests from the project root with -DENABLE_TESTING=ON")
endif()

enable_testing()

# Ensure C++17 for tests
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Python for validation tests
find_package(Python3 COMPONENTS Interpreter REQUIRED)

# ============================================================================
# Find OBS Studio (Standalone logic for tests)
# ============================================================================
if(NOT OBS_INCLUDE_DIRS OR NOT OBS_LIBRARIES)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # Prefer libobs's exported CMake package.  Linux distro pkg-config
        # metadata can lag behind the actual installed OBS runtime/headers.
        find_package(libobs QUIET CONFIG)
        if(TARGET OBS::libobs AND NOT libobs_VERSION VERSION_LESS "32.1.0")
            set(OBS_LIBRARIES OBS::libobs)
            get_target_property(OBS_INCLUDE_DIRS OBS::libobs INTERFACE_INCLUDE_DIRECTORIES)
        else()
            find_package(PkgConfig REQUIRED)
            pkg_check_modules(OBS REQUIRED "libobs >= 32.1.0")
            list(
                APPEND OBS_INCLUDE_DIRS
                ${OBS_INCLUDE_DIRS}
                ${OBS_INCLUDEDIR}
            )
            link_directories(${OBS_LIBRARY_DIRS})
        endif()
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        find_path(
            OBS_INCLUDE_DIR
            obs-module.h
            HINTS
                ${OBS_DIR}
                ENV{OBS_DIR}
                /usr/local/include
                /opt/homebrew/include
            PATH_SUFFIXES obs include include/obs
        )
        find_library(
            OBS_LIBRARY
            obs
            HINTS ${OBS_DIR} ENV{OBS_DIR} /usr/local/lib /opt/homebrew/lib
            PATH_SUFFIXES lib lib64
        )
        if(OBS_INCLUDE_DIR AND OBS_LIBRARY)
            set(OBS_INCLUDE_DIRS ${OBS_INCLUDE_DIR})
            set(OBS_LIBRARIES ${OBS_LIBRARY})
        endif()
    endif()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_path(
        OBS_HEADER_DIR
        obs.h
        HINTS ${OBS_INCLUDE_DIRS}
        PATH_SUFFIXES obs
    )
    if(NOT OBS_HEADER_DIR)
        message(FATAL_ERROR "Could not locate obs.h from include dirs: ${OBS_INCLUDE_DIRS}")
    endif()
    set(OBS_INCLUDE_DIRS ${OBS_HEADER_DIR})
endif()

# Debug prints to verify paths in build log
message(STATUS "Test Include Dirs: ${OBS_INCLUDE_DIRS}")
message(STATUS "Test Libraries:    ${OBS_LIBRARIES}")

# ============================================================================
# Shader Syntax Validation
# ============================================================================
add_test(
    NAME RenderContractTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/test_render_contract.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME RenderPipelineTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/test_render_pipeline.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME GPUResourceLifecycleTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_resources.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

foreach(test_script
        test_filter_chain_dimensions.py
        test_default_preset_contract.py
        test_preset_restraint.py
        test_vidicon_feedback_stability.py
        test_bloom_pyramid.py
        test_bloom_responsiveness.py
        test_effect_responsiveness.py
        test_particle_ui.py
        test_texrender_clear_contract.py)
    get_filename_component(test_name "${test_script}" NAME_WE)
    add_test(
        NAME "${test_name}"
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${test_script}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
    )
endforeach()

add_test(
    NAME ErrorRecoveryTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/test_error_recovery.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME TimeAccumulationTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/test_time_wrap.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME DefaultsAggregatorTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/test_defaults_aggregator.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME ShaderSyntaxTest
    COMMAND
        ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_shader_syntax.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME ShaderIntegrityTest
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_shader_integrity.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME StarburstOrientationTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/test_starburst_orientation.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME VidiconAlphaTest
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_vidicon_alpha.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME VidiconReferenceModelTest
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_vidicon_reference_model.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME VidiconLifecycleContractTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/test_vidicon_lifecycle_contract.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME VidiconLegacyMigrationTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/test_vidicon_legacy_migration.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME IntendedEffectsRecoveryTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/test_intended_effects_recovery.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME CustomizationRangesTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/test_customization_ranges.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME HandheldVertexTest
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_handheld_vertex.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME HandheldMotionModelTest
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_handheld_motion_model.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME BloomLensEffectTest
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_bloom_lens_effect.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME PresetTuningTest
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_preset_tuning.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME GritDefaultsTest
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_grit_defaults.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME SpotDefaultsTest
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_spot_defaults.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME SpotPinholeShaderBehaviorTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/test_spot_pinhole_shader_behavior.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME MirageDefaultsTest
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_mirage_defaults.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME ThreadsafeSettingsDefaultsTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/test_threadsafe_settings_defaults.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME ReviewContractsTest
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_review_contracts.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME MirageAnimationSpeedTest
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_mirage_animation_speed.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME MirageRadiusBoundsTest
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_mirage_radius_bounds.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME ParticleWindLookupTest
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_particle_wind_lookup.py
)
add_test(
    NAME ParticleDefaultsTest
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_particle_defaults.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME CleanupInvariantsTest
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_cleanup_invariants.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME RenderPassthroughContractTest
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_render_passthrough_contract.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME ObsUtilsContractTest
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_obs_utils_contract.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME XMacroDispatchContractTest
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_xmacro_dispatch_contract.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME SettingVisibilityContractTest
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_setting_visibility_contract.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME LoadShaderContractTest
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_load_shader_contract.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(
    NAME ColorHelperContractTest
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_color_helper_contract.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# ============================================================================
# Strategy Source Files
# ============================================================================
set(STRATEGY_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/strategies/strategies.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/strategies/strategy-vidicon.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/strategies/strategy-starburst.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/strategies/strategy-leak.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/strategies/strategy-handheld.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/strategies/strategy-bokeh.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/strategies/strategy-particles.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/strategies/strategy-orton.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/strategies/strategy-grit.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/strategies/strategy-mirage.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/strategies/strategy-spot.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/strategies/strategy-bloom.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/strategies/strategy-pinhole.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/strategies/strategy-polaroid.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/strategies/strategy-daguerreotype.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/strategies/strategy-technicolor.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/strategies/strategy-wet-lens.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/strategies/strategy-phone-wet.c
)

set(PLUGIN_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/lens-core.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/lens-ui.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/obs-utils.c
    ${STRATEGY_SOURCES}
)

# ============================================================================
# Memory Leak Test
# ============================================================================
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(test_mem_leaks test_mem_leaks.cpp ${PLUGIN_SOURCES})

    target_link_options(
        test_mem_leaks
        PRIVATE
            -Wl,--wrap=obs_find_module_file
            -Wl,--wrap=obs_module_text
            -Wl,--wrap=obs_current_module
    )
    target_compile_definitions(test_mem_leaks PRIVATE USE_LINKER_WRAP)

    target_include_directories(
        test_mem_leaks
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../src
            ${CMAKE_CURRENT_SOURCE_DIR}/../src/strategies
            ${OBS_INCLUDE_DIRS}
        )

    target_link_libraries(
        test_mem_leaks
        PRIVATE ${OBS_LIBRARIES} ${CMAKE_DL_LIBS}
    )

    target_compile_definitions(test_mem_leaks PRIVATE PLUGIN_VERSION="test")

    find_program(XVFB_RUN_EXECUTABLE xvfb-run)
    if(XVFB_RUN_EXECUTABLE)
        add_test(
            NAME MemLeakTest
            COMMAND ${XVFB_RUN_EXECUTABLE} -a $<TARGET_FILE:test_mem_leaks>
        )
    else()
        add_test(NAME MemLeakTest COMMAND test_mem_leaks)
    endif()

    set_tests_properties(
        MemLeakTest
        PROPERTIES
            LABELS "integration;requires_graphics;requires_obs_dev"
            ENVIRONMENT "DISABLE_DISPLAY_CHECK=1"
    )
endif()

# ============================================================================
# Strategy Vtable Test
# ============================================================================
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(test_strategies test_strategies.cpp test_strategies_stubs.c ${STRATEGY_SOURCES})

    target_include_directories(
        test_strategies
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../src
            ${CMAKE_CURRENT_SOURCE_DIR}/../src/strategies
            ${OBS_INCLUDE_DIRS}
    )

    target_link_libraries(
        test_strategies
        PRIVATE ${OBS_LIBRARIES} ${CMAKE_DL_LIBS}
    )

    target_compile_definitions(test_strategies PRIVATE PLUGIN_VERSION="test")

    add_test(NAME StrategyVtableTest COMMAND test_strategies)

    set_tests_properties(
        StrategyVtableTest
        PROPERTIES
            LABELS "integration;requires_obs_dev"
    )
endif()

# ============================================================================
# Python Integration Test
# ============================================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_integration.py")
    add_test(
        NAME PythonIntegrationTest
        COMMAND
            ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/test_integration.py --plugin-path
            $<TARGET_FILE:test_mem_leaks>
    )

    set_tests_properties(
        PythonIntegrationTest
        PROPERTIES LABELS "integration;requires_obspython" SKIP_RETURN_CODE 77
    )
endif()

# Genuine OBS/OpenGL shader test. Structural Python contracts are not a
# substitute for actual HLSL/Cg compilation and pixel output.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_program(EMULENS_XVFB_RUN xvfb-run)
    if(EMULENS_XVFB_RUN)
        add_executable(test_wet_lens_gpu test_wet_lens_gpu.cpp)
        target_include_directories(test_wet_lens_gpu PRIVATE ${OBS_INCLUDE_DIRS})
        target_link_libraries(test_wet_lens_gpu PRIVATE ${OBS_LIBRARIES})
        add_test(NAME WetLensNeighborCoverageTest
            COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/test_wet_lens_neighbor_coverage.py)
        add_test(NAME WetLensGpuRenderTest
            COMMAND ${EMULENS_XVFB_RUN} -a -s
                "-screen 0 1280x720x24"
                "$<TARGET_FILE:test_wet_lens_gpu>"
                "${CMAKE_CURRENT_SOURCE_DIR}/../data/emulens.effect"
                "$<TARGET_FILE:emuLensFX>"
                "${CMAKE_CURRENT_SOURCE_DIR}/../data")
        set_tests_properties(WetLensGpuRenderTest PROPERTIES TIMEOUT 30)
        add_executable(test_phone_wet_gpu test_phone_wet_gpu.cpp)
        target_include_directories(test_phone_wet_gpu PRIVATE ${OBS_INCLUDE_DIRS})
        target_link_libraries(test_phone_wet_gpu PRIVATE ${OBS_LIBRARIES})
        add_test(NAME PhoneWetGpuRenderTest
            COMMAND ${EMULENS_XVFB_RUN} -a -s
                "-screen 0 1280x720x24"
                "$<TARGET_FILE:test_phone_wet_gpu>"
                "${CMAKE_CURRENT_SOURCE_DIR}/../data/emulens.effect"
                "$<TARGET_FILE:emuLensFX>"
                "${CMAKE_CURRENT_SOURCE_DIR}/../data")
        set_tests_properties(PhoneWetGpuRenderTest PROPERTIES TIMEOUT 30)
    endif()
endif()
