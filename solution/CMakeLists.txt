cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

add_subdirectory(EasyVulkan_Ch1)
add_subdirectory(EasyVulkan_Ch2)
add_subdirectory(EasyVulkan_Ch6)
add_subdirectory(EasyVulkan_Ch7)
add_subdirectory(EasyVulkan_Ch8)