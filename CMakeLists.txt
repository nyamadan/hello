cmake_minimum_required(VERSION 3.0.0)
project(hello_embree VERSION 0.1.0)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(ExternalProject)

set(EXTERNAL_DIR ${CMAKE_CURRENT_BINARY_DIR}/external)
set(EMBREE_DIR ${EXTERNAL_DIR}/src/embree)
set(EMBREE_LIBRARY_DIR ${EMBREE_DIR}/lib)
set(EMBREE_BINARY_DIR ${EMBREE_DIR}/bin)
set(EMBREE_INCLUDE_DIR ${EMBREE_DIR}/include)
set(TBB_DIR ${EXTERNAL_DIR}/src/tbb/tbb)
set(TBB_LIBRARY_DIR ${TBB_DIR}/lib/intel64/vc14)
set(TBB_BINARY_DIR ${TBB_DIR}/bin/intel64/vc14)
set(TBB_INCLUDE_DIR ${TBB_DIR}/include)
set(OIDN_DIR ${EXTERNAL_DIR}/src/oidn)
set(OIDN_LIBRARY_DIR ${OIDN_DIR}/lib)
set(OIDN_BINARY_DIR ${OIDN_DIR}/bin)
set(OIDN_INCLUDE_DIR ${OIDN_DIR}/include)
set(LUA_DIR ${EXTERNAL_DIR}/src/lua)
set(LUA_INCLUDE_DIR ${LUA_DIR}/src)

ExternalProject_Add(embree
    PREFIX              ${EXTERNAL_DIR}
    URL                 https://github.com/embree/embree/releases/download/v3.10.0/embree-3.10.0.x64.vc14.windows.zip
    BUILD_IN_SOURCE     True
    UPDATE_COMMAND      ""
    CONFIGURE_COMMAND   ""
    BUILD_COMMAND       ""
    INSTALL_COMMAND     COMMAND ${CMAKE_COMMAND} -E copy "${EMBREE_BINARY_DIR}/embree3.dll" "${PROJECT_SOURCE_DIR}/"
)

ExternalProject_Add(tbb
    PREFIX              ${EXTERNAL_DIR}
    URL                 https://github.com/oneapi-src/oneTBB/releases/download/v2020.2/tbb-2020.2-win.zip
    BUILD_IN_SOURCE     True
    UPDATE_COMMAND      ""
    CONFIGURE_COMMAND   ""
    BUILD_COMMAND       ""
    INSTALL_COMMAND     COMMAND ${CMAKE_COMMAND} -E copy "${TBB_BINARY_DIR}/tbb.dll" "${TBB_BINARY_DIR}/tbb_debug.dll" "${PROJECT_SOURCE_DIR}/"
)

ExternalProject_Add(oidn
    PREFIX              ${EXTERNAL_DIR}
    URL                 https://github.com/OpenImageDenoise/oidn/releases/download/v1.2.0/oidn-1.2.0.x64.vc14.windows.zip
    BUILD_IN_SOURCE     True
    UPDATE_COMMAND      ""
    CONFIGURE_COMMAND   ""
    BUILD_COMMAND       ""
    INSTALL_COMMAND     COMMAND ${CMAKE_COMMAND} -E copy "${OIDN_BINARY_DIR}/OpenImageDenoise.dll" "${PROJECT_SOURCE_DIR}/"
)

ExternalProject_Add(lua
    PREFIX              ${EXTERNAL_DIR}
    URL                 http://www.lua.org/ftp/lua-5.4.0.tar.gz
    BUILD_IN_SOURCE     True
    UPDATE_COMMAND      ""
    CONFIGURE_COMMAND   ""
    BUILD_COMMAND       ""
    INSTALL_COMMAND     ""
)

execute_process(
  COMMAND "python" "gl3w_gen.py"
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/gl3w"
)

add_subdirectory("${PROJECT_SOURCE_DIR}/glfw")

file( GLOB_RECURSE LUA_SOURCES ${LUA_DIR}/src/*.c )
list( REMOVE_ITEM LUA_SOURCES
  ${LUA_DIR}/src/lua.c
  ${LUA_DIR}/src/luac.c
)

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
)

link_directories(
    "${EMBREE_LIBRARY_DIR}"
    "${TBB_LIBRARY_DIR}"
    "${OIDN_LIBRARY_DIR}"
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
    "${LUA_SOURCES}"
)

add_definitions(-DGLM_ENABLE_EXPERIMENTAL)
add_dependencies(hello_embree glfw embree tbb oidn lua)
target_link_libraries(hello_embree glfw embree3 OpenImageDenoise)
