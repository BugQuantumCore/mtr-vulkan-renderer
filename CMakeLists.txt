cmake_minimum_required(VERSION 3.20)
project(mtr_vulkan_renderer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 查找Vulkan SDK
find_package(Vulkan REQUIRED)

# JNI查找
find_package(JNI REQUIRED)
include_directories(${JNI_INCLUDE_DIRS})

# GLM (数学库)
include(FetchContent)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.3
)
FetchContent_MakeAvailable(glm)

# volk (Vulkan meta-loader)
FetchContent_Declare(
    volk
    GIT_REPOSITORY https://github.com/zeux/volk.git
    GIT_TAG master
)
FetchContent_MakeAvailable(volk)

# VMA (Vulkan Memory Allocator)
FetchContent_Declare(
    vma
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(vma)

# 原生库源代码
add_library(mtr_vulkan_renderer SHARED
    src/native/src/jni_bridge.cpp
    src/native/src/vulkan_context.cpp
    src/native/src/vulkan_pipeline.cpp
    src/native/src/vulkan_buffer.cpp
    src/native/src/vulkan_shader.cpp
    src/native/src/vulkan_texture.cpp
    src/native/src/mesh_manager.cpp
    src/native/src/train_renderer.cpp
    src/native/src/rail_renderer.cpp
)

target_include_directories(mtr_vulkan_renderer PRIVATE
    src/native/include
    ${JNI_INCLUDE_DIRS}
    ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(mtr_vulkan_renderer PRIVATE
    Vulkan::Vulkan
    volk::volk
    glm
)

# 设置输出目录
set_target_properties(mtr_vulkan_renderer PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# 安装着色器编译工具
find_program(GLSLANG_VALIDATOR glslangValidator)
if (GLSLANG_VALIDATOR)
    # 编译GLSL着色器为SPIR-V
    set(SHADER_SRC_DIR "${CMAKE_SOURCE_DIR}/src/main/resources/shaders")
    set(SHADER_OUT_DIR "${CMAKE_BINARY_DIR}/shaders")
    file(MAKE_DIRECTORY ${SHADER_OUT_DIR})

    file(GLOB SHADER_SOURCES "${SHADER_SRC_DIR}/*.vert" "${SHADER_SRC_DIR}/*.frag")
    foreach(SHADER ${SHADER_SOURCES})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        add_custom_command(
            OUTPUT "${SHADER_OUT_DIR}/${SHADER_NAME}.spv"
            COMMAND ${GLSLANG_VALIDATOR} -V ${SHADER} -o "${SHADER_OUT_DIR}/${SHADER_NAME}.spv"
            DEPENDS ${SHADER}
            COMMENT "Compiling shader ${SHADER_NAME}"
        )
    endforeach()
endif()