cmake_minimum_required(VERSION 3.0.0)
project(hello_embree VERSION 0.1.0)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(ExternalProject)
set(EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty)

set(LUA_RELEASE 5.4.0)
set(EMBREE_RELEASE 3.10.0)
set(TBB_RELEASE 2020.2)
set(OIDN_RELEASE 1.2.0)

set(EMBREE_DIR ${EXTERNAL_DIR}/embree/embree-${EMBREE_RELEASE})
set(EMBREE_LIBRARY_DIR ${EMBREE_DIR}/lib)
set(EMBREE_BINARY_DIR ${EMBREE_DIR}/bin)
set(EMBREE_INCLUDE_DIR ${EMBREE_DIR}/include)

set(TBB_DIR ${EXTERNAL_DIR}/tbb/tbb-${TBB_RELEASE}/tbb)
set(TBB_LIBRARY_DIR ${TBB_DIR}/lib/intel64/vc14)
set(TBB_BINARY_DIR ${TBB_DIR}/bin/intel64/vc14)
set(TBB_INCLUDE_DIR ${TBB_DIR}/include)

set(OIDN_DIR ${EXTERNAL_DIR}/oidn/oidn-${OIDN_RELEASE})
set(OIDN_LIBRARY_DIR ${OIDN_DIR}/lib)
set(OIDN_BINARY_DIR ${OIDN_DIR}/bin)
set(OIDN_INCLUDE_DIR ${OIDN_DIR}/include)

set(LUA_DIR ${EXTERNAL_DIR}/lua/lua-${LUA_RELEASE})
set(LUA_LIBRARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/build/lua-${LUA_RELEASE})
set(LUA_INCLUDE_DIR ${LUA_DIR}/src)

ExternalProject_Add(
    lua-${LUA_RELEASE}
    URL http://www.lua.org/ftp/lua-${LUA_RELEASE}.tar.gz
    DOWNLOAD_DIR        ${CMAKE_CURRENT_BINARY_DIR}/download/lua-${LUA_RELEASE}
    SOURCE_DIR          ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/lua/lua-${LUA_RELEASE}
    BINARY_DIR          ${LUA_LIBRARY_DIR}
    UPDATE_COMMAND      ""
    INSTALL_COMMAND     ""
    PATCH_COMMAND       ${CMAKE_COMMAND} -E copy
                        "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/lua/CMakeLists.txt" <SOURCE_DIR>/CMakeLists.txt
)

ExternalProject_Add(
    embree-${EMBREE_RELEASE}
    URL                 https://github.com/embree/embree/releases/download/v${EMBREE_RELEASE}/embree-${EMBREE_RELEASE}.x64.vc14.windows.zip
    DOWNLOAD_DIR        ${CMAKE_CURRENT_BINARY_DIR}/download/embree-${EMBREE_RELEASE}
    SOURCE_DIR          ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/embree/embree-${EMBREE_RELEASE}
    BUILD_IN_SOURCE     True
    UPDATE_COMMAND      ""
    CONFIGURE_COMMAND   ""
    BUILD_COMMAND       ""
    INSTALL_COMMAND     COMMAND ${CMAKE_COMMAND} -E copy "${EMBREE_BINARY_DIR}/embree3.dll" "${PROJECT_SOURCE_DIR}/"
)

ExternalProject_Add(
    tbb-${TBB_RELEASE}
    URL                 https://github.com/oneapi-src/oneTBB/releases/download/v${TBB_RELEASE}/tbb-${TBB_RELEASE}-win.zip
    DOWNLOAD_DIR        ${CMAKE_CURRENT_BINARY_DIR}/download/tbb-${TBB_RELEASE}
    SOURCE_DIR          ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/tbb/tbb-${TBB_RELEASE}
    BUILD_IN_SOURCE     True
    UPDATE_COMMAND      ""
    CONFIGURE_COMMAND   ""
    BUILD_COMMAND       ""
    INSTALL_COMMAND     COMMAND ${CMAKE_COMMAND} -E copy "${TBB_BINARY_DIR}/tbb.dll" "${TBB_BINARY_DIR}/tbb_debug.dll" "${PROJECT_SOURCE_DIR}/"
)

ExternalProject_Add(
    oidn-${OIDN_RELEASE}
    URL                 https://github.com/OpenImageDenoise/oidn/releases/download/v${OIDN_RELEASE}/oidn-${OIDN_RELEASE}.x64.vc14.windows.zip
    DOWNLOAD_DIR        ${CMAKE_CURRENT_BINARY_DIR}/download/oidn-${OIDN_RELEASE}
    SOURCE_DIR          ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/oidn/oidn-${OIDN_RELEASE}
    BUILD_IN_SOURCE     True
    UPDATE_COMMAND      ""
    CONFIGURE_COMMAND   ""
    BUILD_COMMAND       ""
    INSTALL_COMMAND     COMMAND ${CMAKE_COMMAND} -E copy "${OIDN_BINARY_DIR}/OpenImageDenoise.dll" "${PROJECT_SOURCE_DIR}/"
)

execute_process(
  COMMAND "python" "gl3w_gen.py"
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/gl3w"
)

add_subdirectory("${PROJECT_SOURCE_DIR}/glfw")
add_subdirectory("${PROJECT_SOURCE_DIR}/libyuv")

include_directories(
    "${EMBREE_INCLUDE_DIR}"
    "${TBB_INCLUDE_DIR}"
    "${OIDN_INCLUDE_DIR}"
    "${LUA_INCLUDE_DIR}"
    "${PROJECT_SOURCE_DIR}/glm"
    "${PROJECT_SOURCE_DIR}/stb"
    "${PROJECT_SOURCE_DIR}/gl3w/include"
    "${PROJECT_SOURCE_DIR}/glfw/include"
    "${PROJECT_SOURCE_DIR}/tinygltf"
    "${PROJECT_SOURCE_DIR}/imgui"
    "${PROJECT_SOURCE_DIR}/mikktspace"
    "${PROJECT_SOURCE_DIR}/tinyobjloader"
    "${PROJECT_SOURCE_DIR}/libyuv/include"
)

link_directories(
    "${EMBREE_LIBRARY_DIR}"
    "${TBB_LIBRARY_DIR}"
    "${OIDN_LIBRARY_DIR}"
    "${LUA_LIBRARY_DIR}"
)

add_executable(
    hello_embree
    "${PROJECT_SOURCE_DIR}/main.cpp"
    "${PROJECT_SOURCE_DIR}/image_buffer.cpp"
    "${PROJECT_SOURCE_DIR}/debug_gui.cpp"
    "${PROJECT_SOURCE_DIR}/xorshift128plus.cpp"
    "${PROJECT_SOURCE_DIR}/mesh.cpp"
    "${PROJECT_SOURCE_DIR}/ray_tracer.cpp"
    "${PROJECT_SOURCE_DIR}/ray_tracer_camera.cpp"
    "${PROJECT_SOURCE_DIR}/fps_camera_controller.cpp"
    "${PROJECT_SOURCE_DIR}/mouse_camera_controller.cpp"
    "${PROJECT_SOURCE_DIR}/stb_image.cpp"
    "${PROJECT_SOURCE_DIR}/stb_image_write.cpp"
    "${PROJECT_SOURCE_DIR}/tiny_gltf.cpp"
    "${PROJECT_SOURCE_DIR}/tiny_obj_loader.cpp"
    "${PROJECT_SOURCE_DIR}/gl3w/src/gl3w.c"
    "${PROJECT_SOURCE_DIR}/imgui/imgui.cpp"
    "${PROJECT_SOURCE_DIR}/imgui/imgui_draw.cpp"
    "${PROJECT_SOURCE_DIR}/imgui/imgui_demo.cpp"
    "${PROJECT_SOURCE_DIR}/imgui/imgui_widgets.cpp"
    "${PROJECT_SOURCE_DIR}/imgui/examples/imgui_impl_glfw.cpp"
    "${PROJECT_SOURCE_DIR}/imgui/examples/imgui_impl_opengl3.cpp"
    "${PROJECT_SOURCE_DIR}/mikktspace/mikktspace.c"
)

add_definitions(-DGLM_ENABLE_EXPERIMENTAL -DGLM_FORCE_AVX)
add_dependencies(hello_embree glfw embree-${EMBREE_RELEASE} tbb-${TBB_RELEASE} oidn-${OIDN_RELEASE} lua-${LUA_RELEASE})
target_compile_options(hello_embree PRIVATE /arch:AVX)
target_link_libraries(hello_embree glfw embree3 yuv OpenImageDenoise lua)
