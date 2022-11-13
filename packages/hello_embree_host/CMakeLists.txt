cmake_minimum_required(VERSION 3.20.0)
project(hello_embree_host VERSION 0.1.0)

set(CMAKE_VERBOSE_MAKEFILE True)

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

# args
ExternalProject_Add(args
  PREFIX "${EXTERNALS_ROOT}"
  URL "https://github.com/Taywee/args/archive/refs/tags/6.4.1.tar.gz"
  URL_HASH SHA256=9b3f78208c9cfb06ccbeb7c44903018c8b036b6592a2d9a7d8695d1944060b35
  CMAKE_ARGS
  -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  -DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_TOOLCHAIN_FILE}
  -DARGS_BUILD_EXAMPLE:BOOL=False
  -DARGS_BUILD_UNITTESTS:BOOL=False
)

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
    URL "https://github.com/libsdl-org/SDL/archive/refs/tags/release-2.24.2.tar.gz"
    URL_HASH SHA256=be08ca66d11b1743996cb69a9b6c44771e16084a2dbcb4b61e52d9ac4983f4fc
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
    URL "https://github.com/libsdl-org/SDL/archive/refs/tags/release-2.24.2.tar.gz"
    URL_HASH SHA256=be08ca66d11b1743996cb69a9b6c44771e16084a2dbcb4b61e52d9ac4983f4fc
    BUILD_BYPRODUCTS ${SDL2_LIB} ${SDL2MAIN_LIB}
    CMAKE_ARGS
    -DCMAKE_BUILD_TYPE:PATH=${CMAKE_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    -DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_TOOLCHAIN_FILE}
    -DSDL_STATIC:BOOL=On
    -DSDL_TEST:BOOL=Off
  )
endif()

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
else()
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
else()
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
endif()

ExternalProject_Add(glslang
  PREFIX "${EXTERNALS_ROOT}"
  URL "https://github.com/KhronosGroup/glslang/archive/refs/tags/11.12.0.tar.gz"
  URL_HASH SHA256=7795a97450fecd9779f3d821858fbc2d1a3bf1dd602617d95b685ccbcabc302f
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

# EMBREE3
if(MSVC)
  set(EMBREE3_LIB ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/embree3.lib)
elseif(MINGW)
  set(EMBREE3_LIB ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/embree3.lib)
elseif(EMSCRIPTEN)
  set(EMBREE3_LIB
    ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libembree3.a
    ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/liblexers.a
    ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libmath.a
    ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libsimd.a
    ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libsys.a
    ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libtasking.a
  )
else()
  set(EMBREE3_LIB ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libembree3.a)
endif()

if(MINGW)
  ExternalProject_Add(embree3
    PREFIX "${EXTERNALS_ROOT}"
    URL "https://github.com/embree/embree/releases/download/v3.13.4/embree-3.13.4.x64.vc14.windows.zip"
    URL_HASH SHA256=6c62147c887c2917bd2f6f6bbe5ff33095fa333c38395feae0d881025601a14e
    BUILD_BYPRODUCTS ${EMBREE3_LIB}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND
    ${CMAKE_COMMAND} -E copy_directory ${EXTERNALS_ROOT}/src/embree3/include ${EXTERNALS_ROOT}/include
    && ${CMAKE_COMMAND} -E copy ${EXTERNALS_ROOT}/src/embree3/bin/embree3.dll ${EXTERNALS_ROOT}/bin/embree3.dll
    && ${CMAKE_COMMAND} -E copy ${EXTERNALS_ROOT}/src/embree3/bin/msvcp140.dll ${EXTERNALS_ROOT}/bin/msvcp140.dll
    && ${CMAKE_COMMAND} -E copy ${EXTERNALS_ROOT}/src/embree3/bin/vcruntime140.dll ${EXTERNALS_ROOT}/bin/vcruntime140.dll
    && ${CMAKE_COMMAND} -E copy ${EXTERNALS_ROOT}/src/embree3/lib/embree3.lib ${EXTERNALS_ROOT}/lib/embree3.lib
    && ${CMAKE_COMMAND} -E copy ${EXTERNALS_ROOT}/src/embree3/bin/tbb12.dll ${EXTERNALS_ROOT}/bin/tbb12.dll
  )
elseif(EMSCRIPTEN)
  ExternalProject_Add(embree3
    PREFIX "${EXTERNALS_ROOT}"
    URL "https://github.com/embree/embree/archive/refs/tags/v3.13.4.tar.gz"
    URL_HASH SHA256=e6a8d1d4742f60ae4d936702dd377bc4577a3b034e2909adb2197d0648b1cb35
    BUILD_BYPRODUCTS ${EMBREE3_LIB}
    CMAKE_ARGS
    -DEMBREE_ISPC_SUPPORT=Off
    -DEMBREE_TASKING_SYSTEM=Internal
    -DEMBREE_TUTORIALS=Off
    -DEMBREE_STATIC_LIB=On
    -DCMAKE_BUILD_TYPE:PATH=${CMAKE_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    -DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_TOOLCHAIN_FILE}
  )
else()
  ExternalProject_Add(embree3
    PREFIX "${EXTERNALS_ROOT}"
    URL "https://github.com/embree/embree/archive/refs/tags/v3.13.4.tar.gz"
    URL_HASH SHA256=e6a8d1d4742f60ae4d936702dd377bc4577a3b034e2909adb2197d0648b1cb35
    BUILD_BYPRODUCTS ${EMBREE3_LIB}
    CMAKE_ARGS
    -DEMBREE_ISPC_SUPPORT=Off
    -DEMBREE_TASKING_SYSTEM=TBB
    -DEMBREE_TUTORIALS=Off
    -DEMBREE_STATIC_LIB=Off
    -DCMAKE_BUILD_TYPE:PATH=${CMAKE_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    -DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_TOOLCHAIN_FILE}
  )
endif()

# TBB
if(MSVC)
  if("${CMAKE_BUILD_TYPE}" MATCHES "Debug")
    set(TBB_LIB ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/tbb12_debug.lib)
  else()
    set(TBB_LIB ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/tbb12.lib)
  endif()
elseif(MINGW)
  set(TBB_LIB ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/tbb12.lib)
elseif(EMSCRIPTEN)
  set(TBB_LIB)
else()
  set(TBB_LIB ${EXTERNALS_ROOT}/${CMAKE_INSTALL_LIBDIR}/libtbb12.a)
endif()

if(MINGW)
  ExternalProject_Add(tbb
    PREFIX "${EXTERNALS_ROOT}"
    URL "https://github.com/oneapi-src/oneTBB/releases/download/v2021.5.0/oneapi-tbb-2021.5.0-win.zip"
    URL_HASH SHA256=096c004c7079af89fe990bb259d58983b0ee272afa3a7ef0733875bfe09fcd8e
    BUILD_BYPRODUCTS ${TBB_LIB}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND
    ${CMAKE_COMMAND} -E copy_directory ${EXTERNALS_ROOT}/src/tbb/include ${EXTERNALS_ROOT}/include
    && ${CMAKE_COMMAND} -E copy ${EXTERNALS_ROOT}/src/tbb/lib/intel64/vc14/tbb12.lib ${EXTERNALS_ROOT}/lib/tbb12.lib
    && ${CMAKE_COMMAND} -E copy ${EXTERNALS_ROOT}/src/tbb/lib/intel64/vc14/tbbbind.lib ${EXTERNALS_ROOT}/lib/tbbbind.lib
    && ${CMAKE_COMMAND} -E copy ${EXTERNALS_ROOT}/src/tbb/lib/intel64/vc14/tbbmalloc.lib ${EXTERNALS_ROOT}/lib/tbbmalloc.lib
    && ${CMAKE_COMMAND} -E copy ${EXTERNALS_ROOT}/src/tbb/lib/intel64/vc14/tbbmalloc_proxy.lib ${EXTERNALS_ROOT}/lib/tbbmalloc_proxy.lib
    && ${CMAKE_COMMAND} -E copy ${EXTERNALS_ROOT}/src/tbb/redist/intel64/vc14/tbb12.dll ${EXTERNALS_ROOT}/bin/tbb12.dll
    && ${CMAKE_COMMAND} -E copy ${EXTERNALS_ROOT}/src/tbb/redist/intel64/vc14/tbbmalloc.dll ${EXTERNALS_ROOT}/bin/tbbmalloc.dll
    && ${CMAKE_COMMAND} -E copy ${EXTERNALS_ROOT}/src/tbb/redist/intel64/vc14/tbbmalloc_proxy.dll ${EXTERNALS_ROOT}/bin/tbbmalloc_proxy.dll
  )
elseif(NOT EMSCRIPTEN)
  ExternalProject_Add(tbb
    PREFIX "${EXTERNALS_ROOT}"
    URL "https://github.com/oneapi-src/oneTBB/archive/refs/tags/v2021.5.0.zip"
    URL_HASH SHA256=83ea786c964a384dd72534f9854b419716f412f9d43c0be88d41874763e7bb47
    BUILD_BYPRODUCTS ${TBB_LIB}
    CMAKE_ARGS
    -DTBB_TEST:BOOL=False
    -DTBB_STRICT:BOOL=False
    -DTBB_EXAMPLES:BOOL=False
    -DCMAKE_BUILD_TYPE:PATH=${CMAKE_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    -DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_TOOLCHAIN_FILE}
  )
endif()

if(NOT EMSCRIPTEN)
  add_dependencies(embree3 tbb)
endif()

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
add_library(hello_embree_host_lib "${CORE_SOURCES}")

if(MSVC)
  set_target_properties(hello_embree_host_lib PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
  set_target_properties(hello_embree_host_lib PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR})
  set_target_properties(hello_embree_host_lib PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR})
endif()

# compile options
target_compile_options(hello_embree_host_lib PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)
target_include_directories(hello_embree_host_lib
  PRIVATE
  "externals/include"
  $<BUILD_INTERFACE:${EXTERNALS_ROOT}/${CMAKE_INSTALL_INCLUDEDIR}>
)
if(MSVC OR MINGW)
  target_compile_definitions(hello_embree_host_lib PUBLIC LUA_BUILD_AS_DLL)
endif()
if(RUN_ON_GITHUB_ACTIONS)
  target_compile_definitions(hello_embree_host_lib PRIVATE RUN_ON_GITHUB_ACTIONS)
endif()
if(MSVC OR MINGW)
  add_custom_command(TARGET hello_embree_host_lib POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${EXTERNALS_ROOT}/bin ${PROJECT_BINARY_DIR}
  )
endif()
add_dependencies(hello_embree_host_lib glad args lua sdl2 embree3 glslang spirvcross)

# core executable
if(MSVC OR MINGW)
  add_executable(hello_embree_host
    WIN32
    src/main.cpp
  )
else()
  add_executable(hello_embree_host
    src/main.cpp
  )
endif()

if(MSVC)
  set_target_properties(hello_embree_host PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
  set_target_properties(hello_embree_host PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR})
  set_target_properties(hello_embree_host PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR})
endif()

if(EMSCRIPTEN)
  set_target_properties(hello_embree_host PROPERTIES OUTPUT_NAME "index" SUFFIX ".js")
endif()

# compile options
target_compile_options(hello_embree_host PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)
target_include_directories(hello_embree_host
  PRIVATE
  "externals/include"
  $<BUILD_INTERFACE:${EXTERNALS_ROOT}/${CMAKE_INSTALL_INCLUDEDIR}>
)
if(MSVC OR MINGW)
  target_compile_definitions(hello_embree_host
    PUBLIC
    LUA_BUILD_AS_DLL
  )
endif()
if(RUN_ON_GITHUB_ACTIONS)
  target_compile_definitions(hello_embree_host PRIVATE RUN_ON_GITHUB_ACTIONS)
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

target_link_libraries(hello_embree_host
  ${SDL2MAIN_LIB}
  hello_embree_host_lib
  glad
  ${EMBREE3_LIB}
  ${TBB_LIB}
  ${LUA_LIB}
  ${SDL2_LIB}
  ${SPIRVCROSS_LIB}
  ${GLSLANG_LIB}
  ${OPENGL_LIBRARIES}
  ${PLATFROM_LIBS}
)

if(MINGW)
  target_link_options(hello_embree_host PRIVATE "-static" "-lstdc++")
elseif(EMSCRIPTEN)
  target_link_options(hello_embree_host PRIVATE
    "-sWASM=1"
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
    "--post-js"
    "${PROJECT_SOURCE_DIR}/post.js"
  )
endif()

#
add_dependencies(hello_embree_host hello_embree_host_lib)

# Tests
include(CTest)
enable_testing()

file(GLOB_RECURSE TEST_SOURCES "src/tests/*.cpp" "src/tests/*.c")
add_executable(hello_embree_host_test "${TEST_SOURCES}")

if(MSVC)
  set_target_properties(hello_embree_host_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
  set_target_properties(hello_embree_host_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR})
  set_target_properties(hello_embree_host_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR})
elseif(EMSCRIPTEN)
  set_target_properties(hello_embree_host_test PROPERTIES OUTPUT_NAME "hello_embree_host_test" SUFFIX ".cjs")
endif()

# compile options
if(MSVC OR MINGW)
  target_compile_definitions(hello_embree_host_test
    PUBLIC
    LUA_BUILD_AS_DLL
  )
endif()
target_compile_options(hello_embree_host_test PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)
if(RUN_ON_GITHUB_ACTIONS)
  target_compile_definitions(hello_embree_host_test PRIVATE RUN_ON_GITHUB_ACTIONS)
endif()
target_include_directories(hello_embree_host_test
  PRIVATE
  "externals/include"
  $<BUILD_INTERFACE:${EXTERNALS_ROOT}/${CMAKE_INSTALL_INCLUDEDIR}>
)

# link option
if(MINGW)
  target_link_options(hello_embree_host_test PRIVATE "-static" "-lstdc++")
elseif(EMSCRIPTEN)
  target_link_options(hello_embree_host_test PRIVATE
    "-sWASM=1"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sENVIRONMENT=node"
    "-sSINGLE_FILE=1"
    "-sHEADLESS=1"
  )
endif()

target_link_libraries(
  hello_embree_host_test
  GTest::gtest_main
  hello_embree_host_lib
  glad
  ${EMBREE3_LIB}
  ${TBB_LIB}
  ${LUA_LIB}
  ${SDL2_LIB}
  ${SPIRVCROSS_LIB}
  ${GLSLANG_LIB}
  ${OPENGL_LIBRARIES}
  ${PLATFROM_LIBS}
)

include(GoogleTest)
gtest_discover_tests(hello_embree_host_test DISCOVERY_MODE PRE_TEST)
