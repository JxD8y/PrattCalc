add_executable(PCalc main.cpp)
add_subdirectory("ImGui")
add_subdirectory("PrattParser")

target_link_libraries(PCalc PRIVATE imgui)
target_link_libraries(PCalc PRIVATE PrattLib)