add_executable(PCalc main.cpp "MainWindow.cpp" "BinaryTree.cpp")
add_subdirectory("ImGui")
add_subdirectory("PrattParser")

target_link_libraries(PCalc PRIVATE imgui)
target_link_libraries(PCalc PRIVATE PrattLib)