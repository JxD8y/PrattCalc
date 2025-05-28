add_executable(PCalc main.cpp)
add_subdirectory("ImGui")

target_link_libraries(PCalc PRIVATE imgui)