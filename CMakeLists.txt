cmake_minimum_required(VERSION 4.3.3)

project(vulkan_test0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)


# -- deps --
add_subdirectory(external/imgui)
add_subdirectory(external/libassert)
add_subdirectory(external/vk_bootstrap)
add_subdirectory(external/sdl3 EXCLUDE_FROM_ALL)


# -- source files --
file( GLOB_RECURSE APP_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)


# -- executable --
add_executable(vulkan_test0 ${APP_SRC})

# -- warnings --
target_compile_options(vulkan_test0 PRIVATE
    -Wall
    -Werror
    -Wimplicit-fallthrough
    -Wno-unused
    -Wunused-result
#    -Wfatal-errors
    -fdiagnostics-color=always
    -ffile-prefix-map=${CMAKE_SOURCE_DIR}=. # gives stack traces a relative path to cwd
)


# -- includes --
target_include_directories(vulkan_test0 PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"

    "${CMAKE_CURRENT_SOURCE_DIR}/external/OpenGL-Mathematics"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/stb_image"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/cpptrace"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/libassert"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/magic_enum"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/VulkanMemoryAllocator/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/VulkanMemoryAllocator/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/vk_bootstrap/src"
)

set_target_properties( vulkan_test0 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
)

# TEMPORARY FOR BENCHMARKING COMPILE TIME
#target_compile_options(vulkan_test0 PRIVATE -ftime-report)


# ===========
# range-v3 : to get some missing c++23 ranges in clang22
# ===========
set(CPPTRACE_BUILD_SHARED   OFF)   # static → no runtime .dylib path juggling
set(CPPTRACE_BUILD_TESTING  OFF)
set(MAGIC_ENUM_OPT_BUILD_TESTS OFF)
set(MAGIC_ENUM_OPT_INSTALL     OFF)

include(FetchContent)
FetchContent_Declare(cpptrace   SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/cpptrace)
FetchContent_Declare(magic_enum SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/magic_enum)
#FetchContent_Declare(libassert  SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/libassert)

FetchContent_MakeAvailable(cpptrace)
target_include_directories( vulkan_test0 SYSTEM PRIVATE  ${CMAKE_SOURCE_DIR}/external/range-v3/include)

# libassert pulls magic_enum + cpptrace by name, since its already declared there is no fetch
#FetchContent_MakeAvailable(libassert)


# -- SDL3




# -- CPPTRACE (stack traces)
if (ENABLE_CPPTRACE)
target_link_libraries(vulkan_test0 PRIVATE cpptrace::cpptrace)
endif()


# ===========
# Precompiled header (pch.hpp)
# ===========
target_precompile_headers(vulkan_test0 PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include/pch.hpp"
)
find_package(Vulkan REQUIRED)

target_link_libraries(vulkan_test0 PRIVATE
    imgui
    SDL3::SDL3
    libassert::assert
    Vulkan::Vulkan
    vk-bootstrap::vk-bootstrap
)
# --- Optimization and sanitizers

OPTION(MC_ENABLE_ASAN  "ENABLE ADDRESSSANITIZER" OFF)
OPTION(MC_ENABLE_LSAN  "ENABLE ADDRESSSANITIZER LEAK DETECTION" OFF)
OPTION(MC_ENABLE_TSAN  "ENABLE THREADSANITIZER" OFF)
OPTION(MC_ENABLE_UBSAN "ENABLE UBSAN" OFF)
OPTION(MC_FAST_MATH    "ENABLE -FFAST-MATH" OFF)
OPTION(MC_O1 "ENABLE -O1" OFF)
OPTION(MC_O2 "ENABLE -O2" OFF)
OPTION(MC_O3 "ENABLE -O3" OFF)


if(MC_O1)
    target_compile_options(vulkan_test0 PRIVATE -O1)
endif()

if(MC_O2)
    target_compile_options(vulkan_test0 PRIVATE -O2)
endif()

if(MC_O3)
    target_compile_options(vulkan_test0 PRIVATE -O3)
endif()

if(MC_FAST_MATH)
    target_compile_options(vulkan_test0 PRIVATE -ffast-math)
endif()

if(MC_ENABLE_ASAN OR MC_ENABLE_LSAN)
    target_compile_options(vulkan_test0 PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
    target_link_options(vulkan_test0 PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
endif()

if(MC_ENABLE_TSAN)
    target_compile_options(vulkan_test0 PRIVATE -fsanitize=thread -fno-omit-frame-pointer -g)
    target_link_options(vulkan_test0 PRIVATE -fsanitize=thread -fno-omit-frame-pointer -g)
endif()

if(MC_ENABLE_UBSAN)
    target_compile_options(vulkan_test0 PRIVATE -fsanitize=undefined -fno-omit-frame-pointer -g)
    target_link_options(vulkan_test0 PRIVATE -fsanitize=undefined -fno-omit-frame-pointer -g)
endif()

# helper targets

add_custom_target(run
    COMMAND sh -c "./bin/vulkan_test0"
    DEPENDS vulkan_test0
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_custom_target(debug
    COMMAND lldb -o run -- "$<TARGET_FILE:vulkan_test0>"
    DEPENDS vulkan_test0
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

if(APPLE)
    if(ENABLE_CPPTRACE)
    add_custom_command(TARGET vulkan_test0 POST_BUILD
    COMMAND dsymutil $<TARGET_FILE:vulkan_test0>
    COMMENT "Generating .dSYM for stack traces for cpptrace/libassert")
    endif()
endif()


