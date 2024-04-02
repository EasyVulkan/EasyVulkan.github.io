cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

# 设置项目名称
set(PROJECT_NAME EasyVulkan_Ch2)
project(${PROJECT_NAME})
# C++版本要求
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED)
# 源文件组
file(GLOB NO_GROUP_FILES "*.cpp")
file(GLOB HPP "*.hpp")
file(GLOB HEARDER "*.h")
source_group("" FILES ${NO_GROUP_FILES})
source_group("EasyVK_HPP" FILES ${HPP})
source_group("EasyVK_Header" FILES ${HEARDER})
set(ALL_FILES
    ${NO_GROUP_FILES}
    ${HPP}
    ${HEADER}
)
# 项目生成exe
add_executable(${PROJECT_NAME} ${ALL_FILES})

# 指定输出目录
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY   "${CMAKE_CURRENT_SOURCE_DIR}"
)
# Release下开启优化
set_target_properties(${PROJECT_NAME} PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
)
# 指定附加包含目录
target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_SOURCE_DIR}/Dependency"
    "${CMAKE_SOURCE_DIR}/Dependency/glm"
    "${CMAKE_SOURCE_DIR}/Dependency/Vulkan/Include"
    "${CMAKE_SOURCE_DIR}/Dependency/GLFW/include"
)
# 指定附加库目录
target_link_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_SOURCE_DIR}/Dependency/Vulkan/Lib"
    "${CMAKE_SOURCE_DIR}/Dependency/GLFW/lib_win64"
)

# 预处理器定义
target_compile_definitions(${PROJECT_NAME} PRIVATE
    "$<$<CONFIG:Debug>:"
        "_DEBUG"
    ">"
    "$<$<CONFIG:Release>:"
        "NDEBUG"
    ">"
    "_CONSOLE"
    "UNICODE"
    "_UNICODE"
)

# MSVC编译和链接选项
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:
            /Oi;
            /Gy
        >
        /sdl;
        /W3;
        ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
        ${DEFAULT_CXX_EXCEPTION_HANDLING}
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:
            /INCREMENTAL
        >
        $<$<CONFIG:Release>:
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO
        >
        /DEBUG;
        /SUBSYSTEM:CONSOLE
    )
endif()