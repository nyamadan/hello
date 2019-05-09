cmake_minimum_required(VERSION 3.0.0)

enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(hello_embree VERSION 0.1.0)

if(NOT EMBREE_DIR)
    set(EMBREE_DIR "EMBREE-NOTFOUND" CACHE PATH "Specify Embree3")
    message(FATAL_ERROR "REQUIRED: EMBREE_DIR") 
endif()

if(NOT TBB_INCLUDE_DIR)
    set(TBB_INCLUDE_DIR "TBB-NOTFOUND" CACHE PATH "Specify TBB_INCLUDE_DIR")
    message(FATAL_ERROR "REQUIRED: TBB_INCLUDE_DIR") 
endif()

if(NOT TBB_LIB_DIR)
    set(TBB_LIB_DIR "TBB-NOTFOUND" CACHE PATH "Specify TBB_LIB_DIR")
    message(FATAL_ERROR "REQUIRED: TBB_LIB_DIR") 
endif()

add_subdirectory(
    "${PROJECT_SOURCE_DIR}/glfw"
    "${PROJECT_SOURCE_DIR}/tinygltf"
)

include_directories(
    "${EMBREE_DIR}/include"
    "${TBB_INCLUDE_DIR}"
    "${PROJECT_SOURCE_DIR}/glm"
    "${PROJECT_SOURCE_DIR}/stb"
    "${PROJECT_SOURCE_DIR}/gl3w/include"
    "${PROJECT_SOURCE_DIR}/glfw/include"
    "${PROJECT_SOURCE_DIR}/tinygltf"
)

link_directories(
    "${EMBREE_DIR}/lib"
    "${TBB_LIB_DIR}"
)

add_executable(
    hello_embree
    "${PROJECT_SOURCE_DIR}/main.cpp"
    "${PROJECT_SOURCE_DIR}/gl3w/src/gl3w.c"
)
target_link_libraries(hello_embree
    glfw
    "${EMBREE_DIR}/lib/embree3.lib"
    "${TBB_LIB_DIR}/tbb.lib"
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
