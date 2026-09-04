# Third-party sources carried in Engine/lib, built as static targets and
# aggregated into torque::vendored.

set(_torque_vendored_targets "")

function(torque_add_vendored_lib name)
    cmake_parse_arguments(ARG "" "" "DIRS;INCLUDES;DEFINES;EXCLUDE;LINK" ${ARGN})

    set(_sources "")
    foreach(dir IN LISTS ARG_DIRS)
        file(GLOB _found
            "${ENGINE_LIB_DIR}/${dir}/*.c"
            "${ENGINE_LIB_DIR}/${dir}/*.cc"
            "${ENGINE_LIB_DIR}/${dir}/*.cpp")
        list(APPEND _sources ${_found})
    endforeach()

    foreach(pattern IN LISTS ARG_EXCLUDE)
        list(FILTER _sources EXCLUDE REGEX "${pattern}")
    endforeach()

    if(NOT _sources)
        message(FATAL_ERROR "vendored lib '${name}': no sources matched ${ARG_DIRS}")
    endif()

    add_library(${name} STATIC ${_sources})

    set(_includes "")
    foreach(inc IN LISTS ARG_INCLUDES)
        list(APPEND _includes "${ENGINE_LIB_DIR}/${inc}")
    endforeach()
    target_include_directories(${name} PUBLIC ${_includes})

    if(ARG_DEFINES)
        target_compile_definitions(${name} PUBLIC ${ARG_DEFINES})
    endif()
    if(ARG_LINK)
        target_link_libraries(${name} PUBLIC ${ARG_LINK})
    endif()

    # 2012-era C against a C23-default compiler.
    target_compile_options(${name} PRIVATE
        -Wno-implicit-function-declaration
        -Wno-deprecated-non-prototype
        -Wno-incompatible-pointer-types)

    set_target_properties(${name} PROPERTIES FOLDER "Vendored")
    set(_torque_vendored_targets ${_torque_vendored_targets} ${name} PARENT_SCOPE)
endfunction()

torque_add_vendored_lib(torque_zlib
    DIRS     zlib
    INCLUDES zlib)

# pnggccrd.c/pngvcrd.c are libpng's 32-bit x86 MMX paths; the C fallbacks replace them.
torque_add_vendored_lib(torque_lpng
    DIRS     lpng
    INCLUDES lpng
    EXCLUDE  "pnggccrd\\.c$" "pngvcrd\\.c$"
    LINK     torque_zlib)

torque_add_vendored_lib(torque_ljpeg
    DIRS     ljpeg
    INCLUDES ljpeg)

# Calls open() with O_RDONLY but never includes <fcntl.h>; on Windows that came
# in via <io.h>. Force-included -- SDL3_image retires this library in Phase 6.
torque_add_vendored_lib(torque_lungif
    DIRS     lungif
    INCLUDES lungif)
target_compile_options(torque_lungif PRIVATE -include fcntl.h)

torque_add_vendored_lib(torque_lmng
    DIRS     lmng
    INCLUDES lmng
    DEFINES  MNG_BUILD_SO MNG_SUPPORT_READ MNG_SUPPORT_DISPLAY MNG_SUPPORT_WRITE
    LINK     torque_zlib torque_ljpeg)

torque_add_vendored_lib(torque_squish
    DIRS     squish
    INCLUDES squish)

torque_add_vendored_lib(torque_opcode
    DIRS     opcode opcode/Ice
    INCLUDES opcode
    DEFINES  ICE_NO_DLL BAN_OPCODE_AUTOLINK)

# NvSimpleTypes.h gates its typedefs on `#elif LINUX`, an uppercase macro no
# compiler predefines and the build system was expected to supply.
torque_add_vendored_lib(torque_convexDecomp
    DIRS     convexDecomp
    INCLUDES convexDecomp
    DEFINES  LINUX)

torque_add_vendored_lib(torque_tinyxml
    DIRS     tinyxml
    INCLUDES tinyxml)

torque_add_vendored_lib(torque_libogg
    DIRS     libogg/src
    INCLUDES libogg/include)

torque_add_vendored_lib(torque_libvorbis
    DIRS     libvorbis/lib
    INCLUDES libvorbis/include libvorbis/lib
    LINK     torque_libogg)

# dec/x86 and dec/x86_vc are 32-bit asm; the C path in dec/ supersedes them.
torque_add_vendored_lib(torque_libtheora
    DIRS     libtheora/lib libtheora/lib/dec libtheora/lib/enc
    INCLUDES libtheora/include
    LINK     torque_libogg)

if(TORQUE_COLLADA)
    torque_add_vendored_lib(torque_pcre
        DIRS     pcre
        INCLUDES pcre
        DEFINES  PCRE_STATIC HAVE_CONFIG_H)

    torque_add_vendored_lib(torque_collada_dom
        DIRS     collada/src/1.4/dom
                 collada/src/dae
                 collada/src/modules/stdErrPlugin
                 collada/src/modules/STLDatabase
        INCLUDES collada/include collada/include/1.4
        DEFINES  DOM_INCLUDE_TINYXML PCRE_STATIC
        LINK     torque_pcre torque_tinyxml)
endif()

add_library(torque_vendored INTERFACE)
target_link_libraries(torque_vendored INTERFACE ${_torque_vendored_targets})

# The engine includes these prefixed with the library directory name, e.g.
# "zlib/zlib.h", so Engine/lib itself must be on the path.
target_include_directories(torque_vendored INTERFACE "${ENGINE_LIB_DIR}")
add_library(torque::vendored ALIAS torque_vendored)

message(STATUS "Vendored libraries: ${_torque_vendored_targets}")
