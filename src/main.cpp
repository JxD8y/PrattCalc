#include <iostream>
#include "Imgui_Impl_easy.h"

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"DXGI.lib")

using namespace std;

void HideConsoleWindow() {
	HWND console = GetConsoleWindow();
	ShowWindow(console, SW_HIDE);
}
void MainWindow() {
	ImGui::Begin("PrattCalc",NULL,ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar |ImGuiWindowFlags_NoMove);
	ImVec2 size(800, 600);
	ImGui::SetWindowSize(size);
	ImGui::SetWindowPos(ImVec2(0, 0));
	ImGui::Text("Expression: ");
    ImGui::End();
}
int main() {
	HideConsoleWindow();
	Imeasy::AddRenderingObject(MainWindow);
	Imeasy::StartRendering("WIN",400,500);
}