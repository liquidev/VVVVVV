# CMake File for VVVVVV
# Written by Ethan "flibitijibibo" Lee

cmake_minimum_required(VERSION 2.8.12)

# CMake Options
option(ENABLE_WARNINGS "Enable compilation warnings" ON)
option(ENABLE_WERROR "Treat compilation warnings as errors" OFF)

option(BUNDLE_DEPENDENCIES "Use bundled TinyXML-2, PhysicsFS, and UTF8-CPP (if disabled, TinyXML-2 and PhysicsFS will be dynamically linked, LodePNG and UTF8-CPP will still be statically linked)" ON)

set(CUSTOM_LEVEL_SUPPORT ENABLED CACHE STRING "Optionally disable playing and/or editing of custom levels")
set_property(CACHE CUSTOM_LEVEL_SUPPORT PROPERTY STRINGS ENABLED NO_EDITOR DISABLED)

option(OFFICIAL_BUILD "Compile an official build of the game" OFF)

option(MAKEANDPLAY "Compile a version of the game without the main campaign (provided for convenience; consider modifying MakeAndPlay.h instead" OFF)

if(${CMAKE_VERSION} VERSION_LESS "3.1.3")
    message(WARNING "Your CMake version is too old; set -std=c90 -std=c++98 yourself!")
else()
    set(CMAKE_C_STANDARD 90)
    set(CMAKE_C_EXTENSIONS OFF)

    set(CMAKE_CXX_STANDARD 98)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

project(VVVVVV)

if (NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/psp-toolchain.cmake)
endif()

# Source Lists
set(VVV_SRC
    src/BinaryBlob.cpp
    src/BlockV.cpp
    src/Ent.cpp
    src/Entity.cpp
    src/FileSystemUtils.cpp
    src/Finalclass.cpp
    src/Game.cpp
    src/Graphics.cpp
    src/GraphicsResources.cpp
    src/GraphicsUtil.cpp
    src/Input.cpp
    src/KeyPoll.cpp
    src/Labclass.cpp
    src/Logic.cpp
    src/Map.cpp
    src/Music.cpp
    src/Otherlevel.cpp
    src/preloader.cpp
    src/Render.cpp
    src/RenderFixed.cpp
    src/Screen.cpp
    src/Script.cpp
    src/Scripts.cpp
    src/SoundSystem.cpp
    src/Spacestation2.cpp
    src/TerminalScripts.cpp
    src/Textbox.cpp
    src/Tower.cpp
    src/UtilityClass.cpp
    src/WarpClass.cpp
    src/XMLUtils.cpp
    src/main.cpp
    src/DeferCallbacks.c
    src/GlitchrunnerMode.c
    src/Network.c
    src/ThirdPartyDeps.c
    src/Vlogging.c
    src/Xoshiro.c
    third_party/physfs/extras/physfsrwops.c
)
# NOTE(PSP): Might be able to use this.
if(NOT CUSTOM_LEVEL_SUPPORT STREQUAL "DISABLED")
    list(APPEND VVV_SRC src/CustomLevels.cpp)
    if(NOT CUSTOM_LEVEL_SUPPORT STREQUAL "NO_EDITOR")
        LIST(APPEND VVV_SRC src/Editor.cpp)
    endif()
endif()

add_executable(VVVVVV ${VVV_SRC})

# Include Directories
if(BUNDLE_DEPENDENCIES)
    target_include_directories(
        VVVVVV PRIVATE
        src
        third_party/tinyxml2
        third_party/physfs
        third_party/physfs/extras
        third_party/lodepng
        third_party/utfcpp/source
    )
else()
    target_include_directories(
        VVVVVV PRIVATE
        src
        third_party/lodepng
        third_party/physfs/extras
    )
endif()

if(MAKEANDPLAY)
    target_compile_definitions(VVVVVV PRIVATE -DMAKEANDPLAY)
endif()

set(XML2_SRC
    third_party/tinyxml2/tinyxml2.cpp
)
set(PFS_SRC
    third_party/physfs/physfs.c
    third_party/physfs/physfs_archiver_dir.c
    third_party/physfs/physfs_archiver_unpacked.c
    third_party/physfs/physfs_archiver_zip.c
    third_party/physfs/physfs_byteorder.c
    third_party/physfs/physfs_platform_psp.c
    third_party/physfs/physfs_platform_unix.c
    third_party/physfs/physfs_unicode.c
)
set(PNG_SRC third_party/lodepng/lodepng.c)

if(NOT OFFICIAL_BUILD)
    # Add interim commit hash and its date to the build

    # find_package sets GIT_FOUND and GIT_EXECUTABLE
    find_package(Git)

    if(GIT_FOUND)
        # These filenames have to be qualified, because when we run
        # the CMake script, its work dir gets set to the build folder
        set(VERSION_INPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/src/Version.h.in)
        set(VERSION_OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/src/Version.h.out)

        add_custom_target(
            GenerateVersion ALL
            # This BYPRODUCTS line is required for this to be ran every time
            BYPRODUCTS ${VERSION_OUTPUT_FILE}
            COMMAND ${CMAKE_COMMAND}
            # These args have to be passed through, otherwise the script can't see them
            # Also, these args have to come BEFORE `-P`! (Otherwise it fails with an unclear error)
            -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
            -DINPUT_FILE=${VERSION_INPUT_FILE}
            -DOUTPUT_FILE=${VERSION_OUTPUT_FILE}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/version.cmake
        )

        add_dependencies(VVVVVV GenerateVersion)

        # This lets Version.h know that Version.h.out exists
        add_definitions(-DVERSION_H_OUT_EXISTS)
    endif()
endif()

# Build options
if(ENABLE_WARNINGS)
    # The weird syntax is due to CMake generator expressions.
    # Saves quite a few lines and boilerplate at the price of readability.
    target_compile_options(VVVVVV PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:
            -Wall -Wpedantic $<$<BOOL:${ENABLE_WERROR}>:-Werror>>
        $<$<CXX_COMPILER_ID:MSVC>:
            /W4 $<$<BOOL:${ENABLE_WERROR}>:/WX>>)
endif()

if(CUSTOM_LEVEL_SUPPORT STREQUAL "NO_EDITOR")
    add_definitions(-DNO_EDITOR)
elseif(CUSTOM_LEVEL_SUPPORT STREQUAL "DISABLED")
    add_definitions(-DNO_CUSTOM_LEVELS -DNO_EDITOR)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(SUPPORTS_IMPLICIT_FALLTHROUGH TRUE)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 7.0)
        set(SUPPORTS_IMPLICIT_FALLTHROUGH TRUE)
    else()
        set(SUPPORTS_IMPLICIT_FALLTHROUGH FALSE)
    endif()
else()
    set(SUPPORTS_IMPLICIT_FALLTHROUGH FALSE)
endif()

if(SUPPORTS_IMPLICIT_FALLTHROUGH)
    target_compile_options(VVVVVV PRIVATE -Werror=implicit-fallthrough)
endif()

# # Disable exceptions
# string(REGEX REPLACE "-fexceptions" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")

# # Disable RTTI
# string(REGEX REPLACE "-frtti" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

#--
# Static Libraries
#--

add_library(lodepng-static STATIC ${PNG_SRC})

target_compile_definitions(lodepng-static PRIVATE
    -DLODEPNG_NO_COMPILE_ALLOCATORS
    -DLODEPNG_NO_COMPILE_DISK
    -DLODEPNG_NO_COMPILE_ENCODER
)

add_library(tinyxml2-static STATIC ${XML2_SRC})
add_library(physfs-static STATIC ${PFS_SRC})
target_compile_definitions(physfs-static PRIVATE
    -DPHYSFS_SUPPORTS_DEFAULT=0 -DPHYSFS_SUPPORTS_ZIP=1
)

# PhysFS needs some extensions...
if(${CMAKE_VERSION} VERSION_GREATER "3.1.3"
OR ${CMAKE_VERSION} VERSION_EQUAL "3.1.3")
    set_property(TARGET physfs-static PROPERTY C_EXTENSIONS ON)
endif()

target_link_libraries(VVVVVV physfs-static tinyxml2-static lodepng-static)

#--
# PSP libraries
#--

set(PSPSDK $ENV{PSPSDK})

include_directories(
    ${PSPSDK}/include
    ${PSPSDK}/libc/include
)

target_link_libraries(VVVVVV -L${PSPSDK}/lib -L${PSPSDK}/../lib)

target_link_libraries(VVVVVV
    SDL2_mixer
    SDL2
    vorbisfile
    vorbis
    ogg

    # most of this was taken from davee's infinity CMakeLists
    # https://github.com/DaveeFTW/Infinity/blob/ad6e5e1f752372c46e7bc5e20f3f2e7e736b35f4/app/CMakeLists.txt#L98
    pspaudio
    pspvram
    GL
    psphprm
    c
    pspvfpu
    # pspgum
    pspgu
    psprtc
    # m
    # psphttp
    # pspssl
    pspdebug
    pspdisplay
    pspge
    # pspctrl
    pspsdk
    c
    pspnet
    pspnet_inet
    # pspnet_apctl
    # pspnet_resolver
    psputility
    pspuser
)

set(RES_PARAM_SFO ${CMAKE_CURRENT_SOURCE_DIR}/resources/PARAM.SFO)
set(RES_ICON0 ${CMAKE_CURRENT_SOURCE_DIR}/resources/ICON0.PNG)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CreatePBP.cmake)

create_pbp_file(
    TARGET VVVVVV
    TITLE "VVVVVV"
    ICON_PATH ${RES_ICON0}
    BACKGROUND_PATH NULL
    PREVIEW_PATH NULL
)

# add_custom_command(TARGET VVVVVV
#     POST_BUILD
#     COMMAND psp-fixup-imports VVVVVV
#     COMMAND psp-prxgen VVVVVV vvvvvv.prx
#     COMMAND pack-pbp EBOOT.PBP ${RES_PARAM_SFO} ${RES_ICON0} NULL NULL NULL NULL vvvvvv.prx NULL
#     DEPENDS ${RES_PARAM_SFO} ${RES_ICON0}
#     BYPRODUCTS EBOOT.PBP
# )
