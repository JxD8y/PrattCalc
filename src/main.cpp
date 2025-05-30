#include <iostream>
#include "Imgui_Impl_easy.h"
#include <string>
#include <math.h>
#include "PrattParser/Lexer.h"

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
#define RADIUS 25
void DrawBinaryTree(ImDrawList* draw_list, TreeNode* node, ImVec2 position, float X, float Y) {
    if (node == NULL) return;

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
    draw_list->AddCircleFilled(position, RADIUS, IM_COL32(255, 0, 0, 255));
    draw_list->AddText(ImVec2(position.x - 20, position.y - 13), IM_COL32(255, 255, 255, 255), std::to_string(node->value).c_str());
}

void RenderBinaryTree(TreeNode* root) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 startPosition = ImVec2(240, 150);
    if (root == NULL || (root->left == NULL && root->right == NULL)) {
        draw_list->AddText(ImVec2(startPosition.x - 60 , startPosition.y), IM_COL32(255, 0, 0, 255), "No valid EXPR");
        return;
    }
    DrawBinaryTree(draw_list, root, startPosition, 70, 70);
}

static char buffer[256] = {};
static char b_cmp[256] = {};

TreeNode tn;
void OnExpTextChanged(char* text) {
        
}
void MainWindow() {
	ImGui::Begin("PrattCalc",NULL,ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar |ImGuiWindowFlags_NoMove);
	ImVec2 size(500, 400);
	ImGui::SetWindowSize(size);
	ImGui::SetWindowPos(ImVec2(0, 0));
	ImGui::Text("Expression: ");
    float av_x = ImGui::GetContentRegionAvail().x;
    ImGui::SetNextItemWidth(av_x - 20);
    strcpy_s(b_cmp, sizeof(b_cmp), buffer);
	ImGui::InputText(" ", buffer, sizeof(buffer));

    if (strcmp(b_cmp, buffer) && strcmp(buffer,""))
        OnExpTextChanged(buffer);

    ImGui::Text("Calculated answer: ");
    ImGui::Text("Rendered binary tree: ");
    RenderBinaryTree(&tn);
    ImGui::End();
}

int main() {
    Lexer l("33 + 64.5 * 5 + 2 - 0.8");

	HideConsoleWindow();
	Imeasy::AddRenderingObject(MainWindow);
	Imeasy::StartRendering("WIN",400,500);
}