cmake_minimum_required(VERSION 3.24)
project(OnyxOS LANGUAGES CXX)

if(NOT WIN32)
  message(FATAL_ERROR "OnyxOS UWP must be configured with the Windows SDK on Windows.")
endif()

# Seven launch entries are packaged into ONE APPX. Each entry currently uses
# the shared UWP host scaffold; profile-specific QEMU activation is a later step.
set(ONYXOS_APPS
  "OnyxOS-Launcher"
  "OnyxOS-Windows10"
  "OnyxOS-Windows11"
  "OnyxOS-Debian"
  "OnyxOS-Ubuntu"
  "OnyxOS-ArchLinux"
  "OnyxOS-macOS"
)

foreach(app IN LISTS ONYXOS_APPS)
  add_executable(\${app} WIN32
    src/App.cpp
    src/vm/VirtualMachine.cpp
    src/vm/QemuHostAdapter.cpp
    src/vm/QemuInputBridge.cpp
    src/vm/VhdxProvisioner.cpp
    src/downloads/OsImageDownloader.cpp
  )
  target_compile_features(\${app} PRIVATE cxx_std_20)
  target_compile_definitions(\${app} PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
  # Use the Visual C++ component extensions (C++/CX), not the C++/WinRT projection.
  target_compile_options(\${app} PRIVATE /ZW)
  target_link_libraries(\${app} PRIVATE windowsapp)
  set_target_properties(\${app} PROPERTIES
    OUTPUT_NAME "\${app}"
    VS_GLOBAL_WindowsAppContainer true
    VS_GLOBAL_WindowsPackageType None
    VS_GLOBAL_AppContainerApplication true
    VS_GLOBAL_AppxPackage false
  )
  install(TARGETS \${app} RUNTIME DESTINATION .)
endforeach()
