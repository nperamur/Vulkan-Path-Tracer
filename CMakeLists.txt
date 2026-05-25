cmake_minimum_required(VERSION 4.2)
project(vulkan_pathtracer)

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

add_executable(vulkan_pathtracer Application.cpp Application.h
    shader-pipeline/ShaderPair.cpp
    shader-pipeline/ShaderPair.h
    shader-pipeline/ShaderProgram.cpp
    shader-pipeline/ShaderProgram.h
    shader-pipeline/ShaderPipelineRegistry.cpp
    shader-pipeline/ShaderPipelineRegistry.h
        rendering/Renderer.cpp
        rendering/Renderer.h
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
    raytracing/AccelerationStructureManager.cpp
    raytracing/AccelerationStructureManager.h
    VulkanCommon.cpp
    VulkanCommon.h
    syncronization/BarrierManager.cpp
    syncronization/BarrierManager.h
        image-views/RenderPassImageViewManager.cpp
        image-views/RenderPassImageViewManager.h
        scene/SceneManager.cpp
        scene/SceneManager.h
        rendering/RenderGraph.cpp
        rendering/RenderGraph.h
        ModelLoader.cpp
)
target_link_libraries(vulkan_pathtracer PRIVATE glfw glm)

option(FORCE_VOLK "Force volk even if Vulkan SDK is found" OFF)
if (NOT FORCE_VOLK)
find_package(Vulkan)
endif()

if(Vulkan_FOUND AND NOT FORCE_VOLK)
    message(STATUS "Vulkan SDK found - enabling validation layers")
    target_link_libraries(vulkan_pathtracer PRIVATE Vulkan::Vulkan)
    target_compile_definitions(vulkan_pathtracer PRIVATE VULKAN_SDK_FOUND)
else()
    FetchContent_Declare(
    volk
    GIT_REPOSITORY https://github.com/zeux/volk.git
    GIT_TAG master
    )
    FetchContent_MakeAvailable(volk)

    target_link_libraries(vulkan_pathtracer PRIVATE volk)
    target_compile_definitions(vulkan_pathtracer PRIVATE VK_NO_PROTOTYPES)
endif()

add_dependencies(vulkan_pathtracer shaders)
find_program(GLSLC glslc)

file(GLOB GLSL_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/*.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/*.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/*.rgen
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/*.rmiss
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/*.rchit
)

foreach(GLSL_FILE ${GLSL_FILES})
    get_filename_component(FILE_NAME ${GLSL_FILE} NAME)
    set(SPV_FILE ${CMAKE_BINARY_DIR}/resources/shaders/${FILE_NAME}.spv)
    add_custom_command(
            OUTPUT ${SPV_FILE}
            COMMAND ${GLSLC} ${GLSL_FILE} -g -o ${SPV_FILE} --target-env=vulkan1.3
            DEPENDS ${GLSL_FILE}
    )
    list(APPEND SPV_FILES ${SPV_FILE})
endforeach()

add_custom_target(shaders ALL DEPENDS ${SPV_FILES})

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)

    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
#if(CMAKE_BUILD_TYPE STREQUAL "Debug")
#    message(STATUS "DEBUG DETECTED: Engaging Address Sanitizer...")
#    target_compile_options(vulkan_pathtracer PRIVATE -fsanitize=address)
#    target_link_options(vulkan_pathtracer PRIVATE -fsanitize=address)
#
#else()
#    message(STATUS "RELEASE DETECTED: Maximum Speed Mode (No ASan)")
#endif()

#vma
if(MSVC)
    add_compile_options(/MP)
#    target_compile_options(vulkan_pathtracer PRIVATE /we4700 /we4701)
#    target_compile_options(vulkan_pathtracer PRIVATE /analyze)
#    target_compile_options(vulkan_pathtracer PRIVATE /RTC1)

endif()

include(FetchContent)
FetchContent_Declare(
    VulkanMemoryAllocator
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
    GIT_TAG        master
)
FetchContent_MakeAvailable(VulkanMemoryAllocator)

target_link_libraries(vulkan_pathtracer PRIVATE VulkanMemoryAllocator)

file(COPY resources DESTINATION ${CMAKE_CURRENT_BINARY_DIR})