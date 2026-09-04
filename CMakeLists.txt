cmake_minimum_required(VERSION 3.24)

project(OpenTribes
    VERSION 0.1.0
    DESCRIPTION "Torque3D 1.2 migrated to SDL3 and 64-bit Linux"
    LANGUAGES C CXX)

if(CMAKE_SIZEOF_VOID_P LESS 8)
    message(FATAL_ERROR
        "OpenTribes is 64-bit only. The 32-bit assumptions of the original "
        "Torque3D tree were removed deliberately; do not re-add -m32.")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(TORQUE_APP_NAME "OpenTribes" CACHE STRING "Executable and application name")
option(TORQUE_TOOLS    "Build the in-engine world/GUI editors" ON)
# ON to match the original build (Torque3D.php:122 enabled it for every target).
# It cannot simply be switched off yet: ts/tsShapeConstruct.h needs
# ColladaUtils::ImportOptions, and colladaUtils.h pulls the entire COLLADA DOM
# in with it. Splitting ImportOptions out belongs with the COLLADA removal.
option(TORQUE_COLLADA  "Build in-engine COLLADA import (pulls collada_dom, pcre, tinyxml)" ON)
option(TORQUE_TESTS    "Build the engine's in-tree unit tests" OFF)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)   # types.gcc.h dispatches on the bare `linux` macro
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(ENGINE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Engine/source")
set(ENGINE_LIB_DIR    "${CMAKE_CURRENT_SOURCE_DIR}/Engine/lib")

include(Dependencies)
include(VendoredLibs)
include(EngineSources)

add_executable(${TORQUE_APP_NAME})
target_sources(${TORQUE_APP_NAME} PRIVATE ${TORQUE_ENGINE_SOURCES})

target_include_directories(${TORQUE_APP_NAME} PRIVATE
    "${ENGINE_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/generated")

target_compile_definitions(${TORQUE_APP_NAME} PRIVATE
    TORQUE_UNICODE
    UNICODE
    TORQUE_SHADERGEN
    TORQUE_ADVANCED_LIGHTING
    TORQUE_BASIC_LIGHTING
    TORQUE_OPCODE
    BAN_OPCODE_AUTOLINK
    ICE_NO_DLL
    TORQUE_OGGVORBIS
    TORQUE_OGGTHEORA
    LTM_DESC
    LTC_NO_PROTOTYPES
    $<$<CONFIG:Debug>:TORQUE_DEBUG>
    $<$<NOT:$<BOOL:${TORQUE_TOOLS}>>:TORQUE_PLAYER>
    $<$<BOOL:${TORQUE_COLLADA}>:TORQUE_COLLADA>)

# The 2012 tree predates -Wall being survivable. Silence what is merely noisy and
# keep what actually catches 64-bit defects.
target_compile_options(${TORQUE_APP_NAME} PRIVATE
    -Wno-multichar
    -Wno-invalid-offsetof
    -Wno-write-strings
    -Wno-narrowing
    -Wpointer-arith
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-deprecated-declarations>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-class-memaccess>)

target_link_libraries(${TORQUE_APP_NAME} PRIVATE
    torque::vendored
    SDL3::SDL3
    ${CMAKE_DL_LIBS}
    Threads::Threads)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/torqueConfig.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/torqueConfig.h"
    @ONLY)

set_target_properties(${TORQUE_APP_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/game")

message(STATUS "")
message(STATUS "OpenTribes ${PROJECT_VERSION}")
message(STATUS "  app name      : ${TORQUE_APP_NAME}")
message(STATUS "  build type    : ${CMAKE_BUILD_TYPE}")
message(STATUS "  editor tools  : ${TORQUE_TOOLS}")
message(STATUS "  COLLADA import: ${TORQUE_COLLADA}")
message(STATUS "  engine sources: ${TORQUE_ENGINE_SOURCE_COUNT} files")
message(STATUS "")
