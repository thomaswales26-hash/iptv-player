cmake_minimum_required(VERSION 3.20)
project(iptv_player VERSION 1.0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(iptv-player main.cpp)
target_compile_options(iptv-player PRIVATE -Wall -Wextra -Wpedantic)

if(APPLE)
    enable_language(OBJCXX)
    target_sources(iptv-player PRIVATE macos_launcher.mm)
    set_source_files_properties(macos_launcher.mm PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
    target_link_libraries(iptv-player PRIVATE
        "-framework AppKit"
        "-framework UniformTypeIdentifiers")
    set_target_properties(iptv-player PROPERTIES
        MACOSX_BUNDLE TRUE
        OUTPUT_NAME "IPTV Player"
        MACOSX_BUNDLE_BUNDLE_NAME "IPTV Player"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.iptvplayer.app"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")
endif()

option(IPTV_BUNDLE_FFMPEG "Copy discovered FFmpeg executables beside the player" ON)
if(IPTV_BUNDLE_FFMPEG)
    find_program(IPTV_FFMPEG_EXECUTABLE NAMES ffmpeg)
    find_program(IPTV_FFPLAY_EXECUTABLE NAMES ffplay)

    if(IPTV_FFMPEG_EXECUTABLE AND IPTV_FFPLAY_EXECUTABLE)
        get_filename_component(IPTV_FFMPEG_REALPATH "${IPTV_FFMPEG_EXECUTABLE}" REALPATH)
        get_filename_component(IPTV_FFPLAY_REALPATH "${IPTV_FFPLAY_EXECUTABLE}" REALPATH)

        if(APPLE)
            set_property(TARGET iptv-player APPEND PROPERTY LINK_DEPENDS
                "${CMAKE_CURRENT_SOURCE_DIR}/cmake/BundleFfmpegMacOS.cmake")
            execute_process(COMMAND otool -L "${IPTV_FFPLAY_REALPATH}"
                OUTPUT_VARIABLE IPTV_FFPLAY_DEPENDENCIES
                RESULT_VARIABLE IPTV_OTOOL_RESULT)
            set(IPTV_SDL3_ARGUMENT)
            if(IPTV_OTOOL_RESULT EQUAL 0 AND
               IPTV_FFPLAY_DEPENDENCIES MATCHES "libSDL2-2\\.0\\.0\\.dylib")
                find_library(IPTV_SDL3_LIBRARY NAMES SDL3
                    HINTS /opt/homebrew/opt/sdl3/lib /usr/local/opt/sdl3/lib)
                if(IPTV_SDL3_LIBRARY)
                    get_filename_component(IPTV_SDL3_REALPATH "${IPTV_SDL3_LIBRARY}" REALPATH)
                    list(APPEND IPTV_SDL3_ARGUMENT "-DSDL3_SOURCE=${IPTV_SDL3_REALPATH}")
                else()
                    message(WARNING "FFplay uses SDL2-compat but SDL3 was not found; bundled ffplay may not run")
                endif()
            endif()
            add_custom_command(TARGET iptv-player POST_BUILD
                COMMAND ${CMAKE_COMMAND}
                    "-DOUTPUT_DIR=$<TARGET_FILE_DIR:iptv-player>"
                    "-DFFMPEG_SOURCE=${IPTV_FFMPEG_REALPATH}"
                    "-DFFPLAY_SOURCE=${IPTV_FFPLAY_REALPATH}"
                    ${IPTV_SDL3_ARGUMENT}
                    -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/BundleFfmpegMacOS.cmake"
                VERBATIM)
            install(PROGRAMS
                "$<TARGET_FILE_DIR:iptv-player>/ffmpeg"
                "$<TARGET_FILE_DIR:iptv-player>/ffplay"
                DESTINATION "IPTV Player.app/Contents/MacOS")
            install(DIRECTORY "$<TARGET_FILE_DIR:iptv-player>/../Frameworks/"
                DESTINATION "IPTV Player.app/Contents/Frameworks"
                OPTIONAL)
            message(STATUS "Bundling FFmpeg executables and macOS shared-library dependencies")
        else()
            foreach(tool IN ITEMS IPTV_FFMPEG_REALPATH IPTV_FFPLAY_REALPATH)
                get_filename_component(tool_name "${${tool}}" NAME)
                add_custom_command(TARGET iptv-player POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${${tool}}" "$<TARGET_FILE_DIR:iptv-player>/${tool_name}"
                    VERBATIM)
                install(PROGRAMS "${${tool}}" DESTINATION bin)
            endforeach()
            message(STATUS "Bundling FFmpeg executables (system shared libraries may be required)")
        endif()
    else()
        message(WARNING "Both ffmpeg and ffplay are required for bundling; the player will use PATH or environment overrides")
    endif()
endif()

if(APPLE)
    install(TARGETS iptv-player BUNDLE DESTINATION .)
    install(FILES LICENSE THIRD_PARTY_NOTICES.md
        DESTINATION "IPTV Player.app/Contents/Resources")
else()
    install(TARGETS iptv-player RUNTIME DESTINATION bin)
endif()
