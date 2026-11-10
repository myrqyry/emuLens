# This hook is passed to the OBS 32.1 SDK subprocess, not the plugin build.
# OBS's Swift-only libobs-metal target otherwise has no linker language when
# the pinned source project is configured with Xcode.
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    enable_language(Swift)
endif()
