cmake_minimum_required(VERSION 3.16)
project(Asciixel LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(third_party/freetype-2.14.3 EXCLUDE_FROM_ALL)

set(FFMPEG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/build/ffmpeg-install" CACHE PATH
    "Installation directory of the FFmpeg submodule build")
if(NOT EXISTS "${FFMPEG_ROOT}/include/libavformat/avformat.h" OR
   NOT EXISTS "${FFMPEG_ROOT}/include/libavutil/samplefmt.h")
    message(FATAL_ERROR "FFmpeg installation is incomplete. Run scripts/build-ffmpeg.ps1 (Windows) or scripts/build-ffmpeg.sh (Linux) first, or set FFMPEG_ROOT.")
endif()

foreach(component avformat avcodec avutil swscale)
    find_library(FFMPEG_${component}_LIBRARY NAMES ${component}
        PATHS "${FFMPEG_ROOT}/lib" NO_DEFAULT_PATH REQUIRED)
endforeach()

add_library(asciixel_core STATIC
	src/app/frame_converter.cpp
	src/core/image_sampler.cpp
	src/core/charset_builder.cpp
	src/core/glyph_matcher.cpp
	src/core/grid_layout.cpp
	src/core/ascii_renderer.cpp
	src/config/asciixel_config.cpp
	src/config/config_builder.cpp
	src/io/arg_parser.cpp
	src/io/image_loader.cpp
	src/io/frame_normalizer.cpp
	src/io/png_writer.cpp
	src/io/text_writer.cpp
)

target_link_libraries(asciixel_core PRIVATE freetype
    ${FFMPEG_avformat_LIBRARY}
    ${FFMPEG_avcodec_LIBRARY}
    ${FFMPEG_avutil_LIBRARY}
    ${FFMPEG_swscale_LIBRARY})

target_include_directories(asciixel_core PRIVATE "${FFMPEG_ROOT}/include")

target_include_directories(asciixel_core
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(asciixel src/main.cpp)
target_link_libraries(asciixel PRIVATE asciixel_core)
if(MINGW)
	target_link_options(asciixel PRIVATE -municode)
endif()
file(GLOB FFMPEG_RUNTIME_DLLS "${FFMPEG_ROOT}/bin/*.dll")
if(WIN32 AND FFMPEG_RUNTIME_DLLS)
	set_property(TARGET asciixel APPEND PROPERTY LINK_DEPENDS ${FFMPEG_RUNTIME_DLLS})
	add_custom_command(TARGET asciixel POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FFMPEG_RUNTIME_DLLS}
			$<TARGET_FILE_DIR:asciixel>
		COMMAND_EXPAND_LISTS)
endif()

include(CTest)

if(BUILD_TESTING)
    add_executable(test_frame_converter tests/test_frame_converter.cpp)
    target_link_libraries(test_frame_converter PRIVATE asciixel_core)
    add_test(NAME frame_converter COMMAND test_frame_converter)
    add_executable(test_text_writer tests/test_text_writer.cpp)
    target_link_libraries(test_text_writer PRIVATE asciixel_core)
    add_test(NAME text_writer COMMAND test_text_writer "${CMAKE_CURRENT_BINARY_DIR}/text-writer.txt")
    add_executable(test_arg_parser tests/test_arg_parser.cpp)
    target_link_libraries(test_arg_parser PRIVATE asciixel_core)
    add_test(NAME arg_parser COMMAND test_arg_parser)

    add_executable(test_ascii_renderer tests/test_ascii_renderer.cpp)
    target_link_libraries(test_ascii_renderer PRIVATE asciixel_core)
    add_test(NAME ascii_renderer COMMAND test_ascii_renderer)

    add_executable(test_png_writer tests/test_png_writer.cpp)
    target_include_directories(test_png_writer PRIVATE "${FFMPEG_ROOT}/include")
    target_link_libraries(test_png_writer PRIVATE asciixel_core
        ${FFMPEG_avformat_LIBRARY} ${FFMPEG_avcodec_LIBRARY} ${FFMPEG_avutil_LIBRARY})
    if(WIN32 AND FFMPEG_RUNTIME_DLLS)
        set_property(TARGET test_png_writer APPEND PROPERTY LINK_DEPENDS ${FFMPEG_RUNTIME_DLLS})
        add_custom_command(TARGET test_png_writer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FFMPEG_RUNTIME_DLLS}
                $<TARGET_FILE_DIR:test_png_writer>
            COMMAND_EXPAND_LISTS)
    endif()
    add_test(NAME png_writer COMMAND test_png_writer "${CMAKE_CURRENT_BINARY_DIR}/png-roundtrip.png")
	add_executable(test_image_sampler
		tests/test_image_sampler.cpp
	)
	target_link_libraries(test_image_sampler PRIVATE asciixel_core)

	add_test(NAME image_sampler COMMAND test_image_sampler)

	add_executable(test_image_loader tests/test_image_loader.cpp)
	target_link_libraries(test_image_loader PRIVATE asciixel_core)
	add_test(NAME image_loader COMMAND test_image_loader
		"${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/red_green.png"
		"${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt")

	add_executable(test_frame_normalizer tests/test_frame_normalizer.cpp)
	target_link_libraries(test_frame_normalizer PRIVATE asciixel_core)
	target_include_directories(test_frame_normalizer PRIVATE "${FFMPEG_ROOT}/include")
	add_test(NAME frame_normalizer COMMAND test_frame_normalizer)

	file(GLOB FFMPEG_DLLS "${FFMPEG_ROOT}/bin/*.dll")
	if(FFMPEG_DLLS)
		add_custom_command(TARGET test_image_loader POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FFMPEG_DLLS}
				$<TARGET_FILE_DIR:test_image_loader>
			COMMAND_EXPAND_LISTS)
		add_custom_command(TARGET test_frame_normalizer POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FFMPEG_DLLS}
				$<TARGET_FILE_DIR:test_frame_normalizer>
			COMMAND_EXPAND_LISTS)
	endif()

	add_executable(test_charset_config tests/test_charset_config.cpp)
	target_link_libraries(test_charset_config PRIVATE asciixel_core)
	add_test(NAME charset_config COMMAND test_charset_config)

	add_executable(test_charset_builder tests/test_charset_builder.cpp)
	target_link_libraries(test_charset_builder PRIVATE asciixel_core)
	if(WIN32 AND EXISTS "C:/Windows/Fonts/consola.ttf")
		add_test(NAME charset_builder COMMAND test_charset_builder "C:/Windows/Fonts/consola.ttf")
	endif()

	add_executable(test_glyph_matcher tests/test_glyph_matcher.cpp)
	target_link_libraries(test_glyph_matcher PRIVATE asciixel_core)
	add_test(NAME glyph_matcher COMMAND test_glyph_matcher)

	add_executable(test_grid_layout tests/test_grid_layout.cpp)
	target_link_libraries(test_grid_layout PRIVATE asciixel_core)
	add_test(NAME grid_layout COMMAND test_grid_layout)

	if(WIN32 AND EXISTS "C:/Windows/Fonts/consola.ttf")
		add_test(NAME png_cli COMMAND ${CMAKE_COMMAND}
			-DPROGRAM=$<TARGET_FILE:asciixel>
			-DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/red_green.png
			-P ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_png_cli.cmake)
		add_test(NAME image_cli COMMAND ${CMAKE_COMMAND}
			-DPROGRAM=$<TARGET_FILE:asciixel>
			-DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/red_green.png
			-P ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_image_cli.cmake)
	endif()
endif()
