add_library(imgui STATIC
    imgui.cpp 
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_impl_dx11.cpp
    imgui_impl_win32.cpp
    imgui_impl_easy.cpp
    imgui_stdlib.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
)
target_include_directories(imgui PUBLIC ../../include/ImGui)