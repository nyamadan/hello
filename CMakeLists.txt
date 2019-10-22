cmake_minimum_required(VERSION 3.0.0)

enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(hello_embree VERSION 0.1.0)

if(NOT EMBREE_INCLUDE_DIR)
    set(EMBREE_INCLUDE_DIR "EMBREE_INCLUDE_DIR-NOTFOUND" CACHE PATH "EMBREE_INCLUDE_DIR")
    message(FATAL_ERROR "REQUIRED: EMBREE_INCLUDE_DIR") 
endif()

if(NOT EMBREE_LIBRARY_DIR)
    set(EMBREE_LIBRARY_DIR "EMBREE_LIBRARY_DIR-NOTFOUND" CACHE PATH "EMBREE_LIBRARY_DIR")
    message(FATAL_ERROR "REQUIRED: EMBREE_LIBRARY_DIR") 
endif()

if(NOT TBB_INCLUDE_DIR)
    set(TBB_INCLUDE_DIR "TBB_INCLUDE_DIR-NOTFOUND" CACHE PATH "TBB_INCLUDE_DIR")
    message(FATAL_ERROR "REQUIRED: TBB_INCLUDE_DIR") 
endif()

if(NOT TBB_LIBRARY_DIR)
    set(TBB_LIBRARY_DIR "TBB_LIBRARY_DIR-NOTFOUND" CACHE PATH "TBB_LIBRARY_DIR")
    message(FATAL_ERROR "REQUIRED: TBB_LIBRARY_DIR") 
endif()

execute_process(
  COMMAND "python" "gl3w_gen.py"
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/gl3w"
)

add_subdirectory("${PROJECT_SOURCE_DIR}/glfw")

include_directories(
    "${EMBREE_INCLUDE_DIR}"
    "${TBB_INCLUDE_DIR}"
    "${PROJECT_SOURCE_DIR}/glm"
    "${PROJECT_SOURCE_DIR}/stb"
    "${PROJECT_SOURCE_DIR}/gl3w/include"
    "${PROJECT_SOURCE_DIR}/glfw/include"
    "${PROJECT_SOURCE_DIR}/tinygltf"
    "${PROJECT_SOURCE_DIR}/imgui"
    "${PROJECT_SOURCE_DIR}/nativefiledialog/src/include"
)

link_directories(
    "${EMBREE_LIBRARY_DIR}"
    "${TBB_LIBRARY_DIR}"
)

add_executable(
    hello_embree
    "${PROJECT_SOURCE_DIR}/main.cpp"
    "${PROJECT_SOURCE_DIR}/xorshift128plus.cpp"
    "${PROJECT_SOURCE_DIR}/geometry.cpp"
    "${PROJECT_SOURCE_DIR}/ray_tracer.cpp"
    "${PROJECT_SOURCE_DIR}/ray_tracer_camera.cpp"
    "${PROJECT_SOURCE_DIR}/fps_camera_controller.cpp"
    "${PROJECT_SOURCE_DIR}/mouse_camera_controller.cpp"
    "${PROJECT_SOURCE_DIR}/stb_image.cpp"
    "${PROJECT_SOURCE_DIR}/stb_image_write.cpp"
    "${PROJECT_SOURCE_DIR}/tiny_gltf.cpp"
    "${PROJECT_SOURCE_DIR}/gl3w/src/gl3w.c"
    "${PROJECT_SOURCE_DIR}/imgui/imgui.cpp"
    "${PROJECT_SOURCE_DIR}/imgui/imgui_draw.cpp"
    "${PROJECT_SOURCE_DIR}/imgui/imgui_demo.cpp"
    "${PROJECT_SOURCE_DIR}/imgui/imgui_widgets.cpp"
    "${PROJECT_SOURCE_DIR}/imgui/examples/imgui_impl_glfw.cpp"
    "${PROJECT_SOURCE_DIR}/imgui/examples/imgui_impl_opengl3.cpp"
)

add_definitions(-DGLM_ENABLE_EXPERIMENTAL)

target_link_libraries(hello_embree glfw
    "${EMBREE_LIBRARY_DIR}/embree3.lib"
    "${TBB_LIBRARY_DIR}/tbb.lib"
    "${PROJECT_SOURCE_DIR}/nfd.lib"
)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
