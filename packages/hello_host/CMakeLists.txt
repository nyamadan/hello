cmake_minimum_required(VERSION 3.25.0)
project(hello_host VERSION 0.1.0)

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GNUInstallDirs)
include(ExternalProject)

if(NOT DEFINED CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

set(EXTERNALS_ROOT ${PROJECT_BINARY_DIR}/externals)

if(NOT EMSCRIPTEN)
  find_package(OpenGL REQUIRED)
else()
  set(OPENGL_LIBRARIES)
endif()

# SDL2
if(MSVC)
  if("${CMAKE_BUILD_TYPE}" MATCHES "Debug")
    set(SDL2_LIB ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/SDL2d.lib)
    set(SDL2MAIN_LIB ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/SDL2maind.lib)
  else()
    set(SDL2_LIB ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/SDL2.lib)
    set(SDL2MAIN_LIB ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/SDL2main.lib)
  endif()
elseif(MINGW)
  if("${CMAKE_BUILD_TYPE}" MATCHES "Debug")
    set(SDL2_LIB "${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libSDL2d.dll.a")
    set(SDL2MAIN_LIB "mingw32" "${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libSDL2maind.a")
  else()
    set(SDL2_LIB "${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libSDL2.dll.a")
    set(SDL2MAIN_LIB "mingw32" "${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libSDL2main.a")
  endif()
else()
  if("${CMAKE_BUILD_TYPE}" MATCHES "Debug")
    set(SDL2_LIB ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libSDL2d.a)
    set(SDL2MAIN_LIB ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libSDL2maind.a)
  else()
    set(SDL2_LIB ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libSDL2.a)
    set(SDL2MAIN_LIB ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libSDL2main.a)
  endif()
endif()

if(NOT EMSCRIPTEN)
  ExternalProject_Add(sdl2
    PREFIX "${EXTERNALS_ROOT}"
    URL "https://github.com/libsdl-org/SDL/archive/refs/tags/release-2.26.5.tar.gz"
    URL_HASH SHA256=8f347d4b5adff605098f31fe35c23cdd91482e76edebacb4fa3b83a91465af83
    BUILD_BYPRODUCTS ${SDL2_LIB} ${SDL2MAIN_LIB}
    CMAKE_ARGS
    -DCMAKE_BUILD_TYPE:PATH=${CMAKE_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    -DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_TOOLCHAIN_FILE}
    -DSDL_STATIC:BOOL=Off
    -DSDL_TEST:BOOL=Off
  )
else()
  ExternalProject_Add(sdl2
    PREFIX "${EXTERNALS_ROOT}"
    URL "https://github.com/libsdl-org/SDL/archive/refs/tags/release-2.26.5.tar.gz"
    URL_HASH SHA256=8f347d4b5adff605098f31fe35c23cdd91482e76edebacb4fa3b83a91465af83
    BUILD_BYPRODUCTS ${SDL2_LIB} ${SDL2MAIN_LIB}
    CMAKE_ARGS
    -DCMAKE_BUILD_TYPE:PATH=${CMAKE_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    -DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_TOOLCHAIN_FILE}
    -DSDL_STATIC:BOOL=On
    -DSDL_TEST:BOOL=Off
  )
endif()

# SDL2_image
if(MSVC)
  if("${CMAKE_BUILD_TYPE}" MATCHES "Debug")
    set(SDL2_image_LIB ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/SDL2_image-staticd.lib)
  else()
    set(SDL2_image_LIB ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/SDL2_image-static.lib)
  endif()
else()
  if("${CMAKE_BUILD_TYPE}" MATCHES "Debug")
    set(SDL2_image_LIB ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libSDL2_imaged.a)
  else()
    set(SDL2_image_LIB ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libSDL2_image.a)
  endif()
endif()

ExternalProject_Add(sdl2_image
  PREFIX "${EXTERNALS_ROOT}"
  URL "https://github.com/libsdl-org/SDL_image/archive/refs/tags/release-2.6.2.tar.gz"
  URL_HASH SHA256=5d91ea72b449a161821ef51464d0767efb6fedf7a773f923c43e483dc137e362
  BUILD_BYPRODUCTS ${SDL2_image_LIB}
  CMAKE_ARGS
  -DCMAKE_BUILD_TYPE:PATH=${CMAKE_BUILD_TYPE}
  -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  -DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_TOOLCHAIN_FILE}
  -DBUILD_SHARED_LIBS:BOOL=Off
  -DSDL2IMAGE_SAMPLES:BOOL=Off
  -DSDL2IMAGE_TESTS:BOOL=Off
  -DSDL2_LIBRARY:PATH=${SDL2_LIB}
  -DSDL2_INCLUDE_DIR:PATH=${EXTERNALS_ROOT}/include/SDL2
)
add_dependencies(sdl2_image sdl2)

# SPIRVCROSS
if(MSVC)
  if("${CMAKE_BUILD_TYPE}" MATCHES "Debug")
    set(SPIRVCROSS_LIB
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/spirv-cross-cored.lib
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/spirv-cross-glsld.lib
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/spirv-cross-hlsld.lib
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/SPIRVd.lib
    )
  else()
    set(SPIRVCROSS_LIB
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/spirv-cross-core.lib
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/spirv-cross-glsl.lib
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/spirv-cross-hlsl.lib
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/SPIRV.lib
    )
  endif()
elseif(MINGW)
  if("${CMAKE_BUILD_TYPE}" MATCHES "Debug")
    set(SPIRVCROSS_LIB
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libspirv-cross-cored.a
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libspirv-cross-glsld.a
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libspirv-cross-hlsld.a
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libSPIRVd.a
    )
  else()
    set(SPIRVCROSS_LIB
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libspirv-cross-core.a
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libspirv-cross-glsl.a
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libspirv-cross-hlsl.a
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libSPIRV.a
    )
  endif()
else()
  set(SPIRVCROSS_LIB
    ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libspirv-cross-core.a
    ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libspirv-cross-glsl.a
    ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libspirv-cross-hlsl.a
    ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libSPIRV.a
  )
endif()

ExternalProject_Add(spirvcross
  PREFIX "${EXTERNALS_ROOT}"
  URL "https://github.com/KhronosGroup/SPIRV-Cross/archive/refs/tags/sdk-1.3.231.1.tar.gz"
  URL_HASH SHA256=3b42f5b6e46b45600e09fd55234f59edb7cfca803e49d7830dc6fb5a086143b1
  BUILD_BYPRODUCTS ${SPIRVCROSS_LIB}
  CMAKE_ARGS
  -DCMAKE_BUILD_TYPE:PATH=${CMAKE_BUILD_TYPE}
  -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  -DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_TOOLCHAIN_FILE}
  -DSPIRV_CROSS_CLI:BOOL=Off
)

# glslang
if(MSVC)
  if("${CMAKE_BUILD_TYPE}" MATCHES "Debug")
    set(GLSLANG_LIB
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/GenericCodeGend.lib
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/glslangd.lib
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/HLSLd.lib
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/MachineIndependentd.lib
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/OGLCompilerd.lib
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/OSDependentd.lib
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/SPIRVd.lib
    )
  else()
    set(GLSLANG_LIB
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/GenericCodeGen.lib
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/glslang.lib
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/HLSL.lib
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/MachineIndependent.lib
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/OGLCompiler.lib
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/OSDependent.lib
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/SPIRV.lib
    )
  endif()
elseif(MINGW)
  if("${CMAKE_BUILD_TYPE}" MATCHES "Debug")
    set(GLSLANG_LIB
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libGenericCodeGend.a
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libglslangd.a
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libHLSLd.a
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libMachineIndependentd.a
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libOGLCompilerd.a
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libOSDependentd.a
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libSPIRVd.a
    )
  else()
    set(GLSLANG_LIB
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libGenericCodeGen.a
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libglslang.a
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libHLSL.a
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libMachineIndependent.a
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libOGLCompiler.a
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libOSDependent.a
      ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libSPIRV.a
    )
  endif()
else()
  set(GLSLANG_LIB
    ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libGenericCodeGen.a
    ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libglslang.a
    ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libHLSL.a
    ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libMachineIndependent.a
    ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libOGLCompiler.a
    ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libOSDependent.a
    ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libSPIRV.a
  )
endif()

ExternalProject_Add(glslang
  PREFIX "${EXTERNALS_ROOT}"
  URL "https://github.com/KhronosGroup/glslang/archive/refs/tags/12.2.0.tar.gz"
  URL_HASH SHA256=870d17030fda7308c1521fb2e01a9e93cbe4b130bc8274e90d00e127432ab6f6
  BUILD_BYPRODUCTS ${GLSLANG_LIB}
  CMAKE_ARGS
  -DCMAKE_BUILD_TYPE:PATH=${CMAKE_BUILD_TYPE}
  -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  -DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_TOOLCHAIN_FILE}
  -DENABLE_SPVREMAPPER:BOOL=Off
  -DENABLE_GLSLANG_BINARIES:BOOL=Off
  -DENABLE_GLSLANG_JS:BOOL=Off
  -DENABLE_CTEST:BOOL=Off
)

# lua
if(MSVC)
  if("${CMAKE_BUILD_TYPE}" MATCHES "Debug")
    set(LUA_LIB ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/luad.lib)
  else()
    set(LUA_LIB ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/lua.lib)
  endif()
elseif(MINGW)
  if("${CMAKE_BUILD_TYPE}" MATCHES "Debug")
    set(LUA_LIB ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libluad.dll.a)
  else()
    set(LUA_LIB ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/liblua.dll.a)
  endif()
else()
  if("${CMAKE_BUILD_TYPE}" MATCHES "Debug")
    set(LUA_LIB ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libluad.a)
  else()
    set(LUA_LIB ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/liblua.a)
  endif()
endif()

ExternalProject_Add(lua
  PREFIX "${EXTERNALS_ROOT}"
  URL https://www.lua.org/ftp/lua-5.4.4.tar.gz
  URL_HASH SHA256=164c7849653b80ae67bec4b7473b884bf5cc8d2dca05653475ec2ed27b9ebf61
  BUILD_BYPRODUCTS ${LUA_LIB}
  CMAKE_ARGS
  -DCMAKE_BUILD_TYPE:PATH=${CMAKE_BUILD_TYPE}
  -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  -DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_TOOLCHAIN_FILE}
  PATCH_COMMAND
  ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/externals/Lua.cmake" <SOURCE_DIR>/CMakeLists.txt
)
link_directories(${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR})

# glad library
file(GLOB_RECURSE EXTERNALS_SOURCES "externals/*.cpp" "externals/*.c")
add_library(glad "${EXTERNALS_SOURCES}")
target_include_directories(glad
  PRIVATE
  "externals/include"
)

file(GLOB_RECURSE CORE_SOURCES "src/core/*.cpp" "src/core/*.c")

# core library
add_library(hello_host_lib "${CORE_SOURCES}")

if(MSVC)
  set_target_properties(hello_host_lib PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
  set_target_properties(hello_host_lib PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR})
  set_target_properties(hello_host_lib PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR})
endif()

# compile options
target_compile_options(hello_host_lib PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)
target_include_directories(hello_host_lib
  PRIVATE
  "externals/include"
  $<BUILD_INTERFACE:${EXTERNALS_ROOT}/${CMAKE_INSTALL_INCLUDEDIR}>
)

if(MSVC OR MINGW)
  target_compile_definitions(hello_host_lib PUBLIC LUA_BUILD_AS_DLL)
endif()

if(RUN_ON_GITHUB_ACTIONS)
  target_compile_definitions(hello_host_lib PRIVATE RUN_ON_GITHUB_ACTIONS)
endif()

if(MSVC OR MINGW)
  add_custom_command(TARGET hello_host_lib POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${EXTERNALS_ROOT}/bin ${PROJECT_BINARY_DIR}
  )
endif()

add_dependencies(hello_host_lib glad lua sdl2 sdl2_image glslang spirvcross)

# core executable
if(MSVC OR MINGW)
  add_executable(hello_host
    WIN32
    src/main.cpp
  )
else()
  add_executable(hello_host
    src/main.cpp
  )
endif()

if(MSVC)
  set_target_properties(hello_host PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
  set_target_properties(hello_host PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR})
  set_target_properties(hello_host PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR})
endif()

if(EMSCRIPTEN)
  set_target_properties(hello_host PROPERTIES OUTPUT_NAME "index" SUFFIX ".js")
endif()

# compile options
target_compile_options(hello_host PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)
target_include_directories(hello_host
  PRIVATE
  "externals/include"
  $<BUILD_INTERFACE:${EXTERNALS_ROOT}/${CMAKE_INSTALL_INCLUDEDIR}>
)

if(MSVC OR MINGW)
  target_compile_definitions(hello_host
    PUBLIC
    LUA_BUILD_AS_DLL
  )
endif()

if(RUN_ON_GITHUB_ACTIONS)
  target_compile_definitions(hello_host PRIVATE RUN_ON_GITHUB_ACTIONS)
endif()

# link option
if(MSVC)
  set(PLATFROM_LIBS
    winmm.lib
    imm32.lib
    setupapi.lib
    version.lib
  )
elseif(MINGW)
  set(PLATFROM_LIBS
    winmm
    imm32
    setupapi
    version
  )
else()
  set(PLATFROM_LIBS)
endif()

target_link_libraries(hello_host
  ${SDL2MAIN_LIB}
  hello_host_lib
  glad
  ${LUA_LIB}
  ${SDL2_image_LIB}
  ${SDL2_LIB}
  ${SPIRVCROSS_LIB}
  ${GLSLANG_LIB}
  ${OPENGL_LIBRARIES}
  ${PLATFROM_LIBS}
)

if(MINGW)
  target_link_options(hello_host PRIVATE "-static" "-lstdc++")
elseif(EMSCRIPTEN)
  target_link_options(hello_host PRIVATE
    "-sWASM=1"
    "-sFETCH=1"
    "-sMAX_WEBGL_VERSION=2"
    "-sMIN_WEBGL_VERSION=2"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sENVIRONMENT=web"
    "-sMODULARIZE=1"
    "-sEXPORT_ES6=1"
    "-sSINGLE_FILE=1"
    "-sNO_EXIT_RUNTIME=1"
    "-sFORCE_FILESYSTEM=1"
    "-sINVOKE_RUN=0"
    "-sEXPORTED_RUNTIME_METHODS=['FS', 'callMain']"
  )
endif()

#
add_dependencies(hello_host hello_host_lib)

# Tests
include(CTest)
enable_testing()

file(GLOB_RECURSE TEST_SOURCES "src/tests/*.cpp" "src/tests/*.c")
add_executable(hello_host_test "${TEST_SOURCES}")

if(MSVC)
  set_target_properties(hello_host_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
  set_target_properties(hello_host_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR})
  set_target_properties(hello_host_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR})
elseif(EMSCRIPTEN)
  set_target_properties(hello_host_test PROPERTIES OUTPUT_NAME "hello_host_test" SUFFIX ".cjs")
endif()

# compile options
if(MSVC OR MINGW)
  target_compile_definitions(hello_host_test
    PUBLIC
    LUA_BUILD_AS_DLL
  )
endif()

target_compile_options(hello_host_test PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)

if(RUN_ON_GITHUB_ACTIONS)
  target_compile_definitions(hello_host_test PRIVATE RUN_ON_GITHUB_ACTIONS)
endif()

target_include_directories(hello_host_test
  PRIVATE
  "externals/include"
  $<BUILD_INTERFACE:${EXTERNALS_ROOT}/${CMAKE_INSTALL_INCLUDEDIR}>
)

# link option
if(MINGW)
  target_link_options(hello_host_test PRIVATE "-static" "-lstdc++")
elseif(EMSCRIPTEN)
  target_link_options(hello_host_test PRIVATE
    "-sWASM=1"
    "-sFETCH=1"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sENVIRONMENT=node"
    "-sSINGLE_FILE=1"
    "-sHEADLESS=1"
    "-sERROR_ON_UNDEFINED_SYMBOLS=0"
  )
endif()

target_link_libraries(
  hello_host_test
  GTest::gtest_main
  hello_host_lib
  glad
  ${LUA_LIB}
  ${SDL2_image_LIB}
  ${SDL2_LIB}
  ${SPIRVCROSS_LIB}
  ${GLSLANG_LIB}
  ${OPENGL_LIBRARIES}
  ${PLATFROM_LIBS}
)

include(GoogleTest)
gtest_discover_tests(hello_host_test DISCOVERY_MODE PRE_TEST WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/../hello_script/public")
