cmake_minimum_required(VERSION 3.0.0)
project(hello_embree VERSION 0.1.0)

if(NOT EMBREE_DIR)
    set(EMBREE_DIR "EMBREE-NOTFOUND" CACHE PATH "Embree")
    message(FATAL_ERROR "REQUIRED: EMBREE_DIR") 
endif()

include_directories("${EMBREE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}/glm")
link_directories("${EMBREE_DIR}/lib")

add_executable(hello_embree main.cpp)
target_link_libraries(hello_embree embree3)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
