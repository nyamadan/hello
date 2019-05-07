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

if(NOT TBB_DIR)
    set(TBB_DIR "TBB-NOTFOUND" CACHE PATH "Specify TBB")
    message(FATAL_ERROR "REQUIRED: TBB_DIR") 
endif()

include_directories(
    "${EMBREE_DIR}/include"
    "${TBB_DIR}/tbb/include"
    "${PROJECT_SOURCE_DIR}/glm"
    "${PROJECT_SOURCE_DIR}/stb"
)

link_directories(
    "${EMBREE_DIR}/lib"
    "${TBB_DIR}/tbb/lib/intel64/vc14"
)

add_executable(hello_embree main.cpp)
target_link_libraries(hello_embree
    "${EMBREE_DIR}/lib/embree3.lib"
    "${TBB_DIR}/tbb/lib/intel64/vc14/tbb.lib"
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
