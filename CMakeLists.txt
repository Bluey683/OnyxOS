cmake_minimum_required(VERSION 3.24)
project(OnyxOS LANGUAGES CXX)

if(NOT WIN32)
  message(FATAL_ERROR "OnyxOS UWP must be configured with the Windows SDK on Windows.")
endif()

add_executable(OnyxOS WIN32 src/App.cpp)
target_compile_features(OnyxOS PRIVATE cxx_std_20)
target_compile_definitions(OnyxOS PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
target_link_libraries(OnyxOS PRIVATE windowsapp)

set_target_properties(OnyxOS PROPERTIES
  OUTPUT_NAME "OnyxOS"
  VS_GLOBAL_WindowsAppContainer true
  VS_GLOBAL_WindowsPackageType None
  VS_GLOBAL_AppContainerApplication true
  VS_GLOBAL_AppxPackage false
)
install(TARGETS OnyxOS RUNTIME DESTINATION .)
