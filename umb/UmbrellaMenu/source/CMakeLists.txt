cmake_minimum_required(VERSION 3.16)
project(UmbrellaMenu CXX)
set(CMAKE_CXX_STANDARD 17)
set(IMGUI ${CMAKE_SOURCE_DIR}/imgui)
add_executable(UmbrellaMenu WIN32
    src/main_win32.cpp src/menu.cpp src/font_data.cpp
    ${IMGUI}/imgui.cpp ${IMGUI}/imgui_draw.cpp ${IMGUI}/imgui_tables.cpp ${IMGUI}/imgui_widgets.cpp
    ${IMGUI}/backends/imgui_impl_win32.cpp ${IMGUI}/backends/imgui_impl_dx11.cpp)
target_include_directories(UmbrellaMenu PRIVATE src ${IMGUI} ${IMGUI}/backends)
target_link_libraries(UmbrellaMenu PRIVATE d3d11 dxgi d3dcompiler dwmapi)
if(MSVC)
  target_compile_options(UmbrellaMenu PRIVATE /utf-8)
endif()
