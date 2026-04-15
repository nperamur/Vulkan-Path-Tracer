cmake_minimum_required(VERSION 4.2)
project(vulkan_test)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

# GLFW
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
)

# GLM
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG master
)
FetchContent_MakeAvailable(glm)

FetchContent_MakeAvailable(glfw)

add_executable(vulkan_test Application.cpp Application.h
        shader-pipeline/ShaderPair.cpp
        shader-pipeline/ShaderPair.h
        shader-pipeline/ShaderProgram.cpp
        shader-pipeline/ShaderProgram.h
        shader-pipeline/ShaderPipelineRegistry.cpp
        shader-pipeline/ShaderPipelineRegistry.h
        Renderer.cpp
        Renderer.h
        Loader.cpp
        Loader.h
        VmaImpl.cpp
        VmaImpl.h
        Entity.cpp
        Entity.h
        Camera.cpp
        Camera.h
        ModelLoader.cpp
        ModelLoader.h
        shader-pipeline/ShaderPipeline.cpp
        shader-pipeline/ShaderPipeline.h
        shader-pipeline/RaytracingShaderPipeline.cpp
        shader-pipeline/RaytracingShaderPipeline.h
)
target_link_libraries(vulkan_test PRIVATE glfw glm)

option(FORCE_VOLK "Force volk even if Vulkan SDK is found" OFF)
if (NOT FORCE_VOLK)
    find_package(Vulkan)
endif()

if(Vulkan_FOUND AND NOT FORCE_VOLK)
    message(STATUS "Vulkan SDK found - enabling validation layers")
    target_link_libraries(vulkan_test PRIVATE Vulkan::Vulkan)
    target_compile_definitions(vulkan_test PRIVATE VULKAN_SDK_FOUND)
else()
    FetchContent_Declare(
            volk
            GIT_REPOSITORY https://github.com/zeux/volk.git
            GIT_TAG master
    )
    FetchContent_MakeAvailable(volk)

    target_link_libraries(vulkan_test PRIVATE volk)
    target_compile_definitions(vulkan_test PRIVATE VK_NO_PROTOTYPES)
endif()

add_dependencies(vulkan_test shaders)
find_program(GLSLC glslc)

file(GLOB GLSL_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/*.vert
        ${CMAKE_CURRENT_SOURCE_DIR}/*.frag
        ${CMAKE_CURRENT_SOURCE_DIR}/*.rgen
        ${CMAKE_CURRENT_SOURCE_DIR}/*.rmiss
        ${CMAKE_CURRENT_SOURCE_DIR}/*.rchit

)

foreach(GLSL_FILE ${GLSL_FILES})
    get_filename_component(FILE_NAME ${GLSL_FILE} NAME)
    set(SPV_FILE ${CMAKE_BINARY_DIR}/${FILE_NAME}.spv)
    add_custom_command(
            OUTPUT ${SPV_FILE}
            COMMAND ${GLSLC} ${GLSL_FILE} -o ${SPV_FILE}
            DEPENDS ${GLSL_FILE}
    )
    list(APPEND SPV_FILES ${SPV_FILE})
endforeach()

add_custom_target(shaders ALL DEPENDS ${SPV_FILES})

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
target_compile_options(vulkan_test PRIVATE -fsanitize=address)
target_link_options(vulkan_test PRIVATE -fsanitize=address)

#vma
include(FetchContent)
FetchContent_Declare(
        VulkanMemoryAllocator
        GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
        GIT_TAG        master
)
FetchContent_MakeAvailable(VulkanMemoryAllocator)

target_link_libraries(vulkan_test PRIVATE VulkanMemoryAllocator)

file(COPY resources DESTINATION ${CMAKE_CURRENT_BINARY_DIR})