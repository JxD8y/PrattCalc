#include <iostream>
#include "Imgui_Impl_easy.h"
#include <string>

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"DXGI.lib")

using namespace std;

void HideConsoleWindow() {
	HWND console = GetConsoleWindow();
	ShowWindow(console, SW_HIDE);
}

struct TreeNode {
    int value;

    TreeNode* left;
    TreeNode* right;
    ImVec2 position;
};

void DrawBinaryTree(ImDrawList* draw_list, TreeNode* node, ImVec2 position, float X, float Y) {
    if (node == NULL) return;

    draw_list->AddCircleFilled(position, 30, IM_COL32(255, 0, 0, 255));
    draw_list->AddText(ImVec2(position.x - 20,position.y-13), IM_COL32(255, 255, 255, 255), std::to_string(node->value).c_str());

    ImVec2 lPos = ImVec2(position.x - X, position.y + Y);
    ImVec2 rPos = ImVec2(position.x + X, position.y + Y);

    if (node->left) {
        draw_list->AddLine(position, lPos, IM_COL32(255, 255, 255, 255), 2.0f);
        DrawBinaryTree(draw_list, node->left, lPos, X / 2, Y);
    }
    if (node->right) {
        draw_list->AddLine(position, rPos, IM_COL32(255, 255, 255, 255), 2.0f);
        DrawBinaryTree(draw_list, node->left, rPos, X / 2, Y);
    }
}

void RenderBinaryTree(TreeNode* root) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 startPosition = ImVec2(200, 130);
    DrawBinaryTree(draw_list, root, startPosition, 50, 70);
}

static char buffer[256] = {};
TreeNode tn, tr, tl;
void MainWindow() {
	ImGui::Begin("PrattCalc",NULL,ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar |ImGuiWindowFlags_NoMove);
	ImVec2 size(500, 400);
	ImGui::SetWindowSize(size);
	ImGui::SetWindowPos(ImVec2(0, 0));
	ImGui::Text("Expression: ");
	ImGui::InputText(" ", buffer, sizeof(buffer));
    tn.value = 10;
    tr.value = 0;
    tl.value = 199;
    tn.right = &tr;
    tn.left = &tl;
    RenderBinaryTree(&tn);
    ImGui::End();
}
int main() {
	HideConsoleWindow();
	Imeasy::AddRenderingObject(MainWindow);
	Imeasy::StartRendering("WIN",400,500);
}