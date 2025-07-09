/// <summary>
/// This file contains codes for a custom binary tree witch can render the Expression object returned by Pratt parser library
/// this tree impl supports zoom with CTRL + SCR also i will dynamically change its rel-positions
/// 
/// JXD8Y
/// </summary>
#include <BinaryTree.h>
float tree_zoom_scale = 1.0f;

int CalculateSubTreeWidth(Expression* node) {
    if (!node)
        return 0;
    int currentWidth = RADIUS / 3 + CalculateSubTreeWidth(node->pLhs) + CalculateSubTreeWidth(node->pRhs);
    return currentWidth * tree_zoom_scale;
}

void DrawBinaryTree(ImDrawList* draw_list, Expression* node, ImVec2 position, float X, float Y) {
    if (node == NULL) return;
    int dRX = CalculateSubTreeWidth(node->pRhs);
    int dLX = CalculateSubTreeWidth(node->pLhs);

    ImVec2 lPos = ImVec2(position.x - X - dLX, position.y + Y + dLX / 8);
    ImVec2 rPos = ImVec2(position.x + X + dRX, position.y + Y + dRX / 8);

    int radius = RADIUS * tree_zoom_scale / 1.5;
    if (radius < CIRCLE_MIN_RADIUS)
        radius = CIRCLE_MIN_RADIUS;

    if (node->pRhs) {
        draw_list->AddLine(position, rPos, ::TXT_COL, 2.0f);
        ImGui::Dummy(ImVec2(0, 70 * tree_zoom_scale));
        DrawBinaryTree(draw_list, node->pRhs, rPos, dRX, Y + dRX / 8);
    }

    if (node->pLhs) {
        draw_list->AddLine(position, lPos, ::TXT_COL, 2.0f);
        ImGui::Dummy(ImVec2(0, 70 * tree_zoom_scale));

        DrawBinaryTree(draw_list, node->pLhs, lPos, dLX, Y + dLX / 8);
    }

    if (node->GetType() == OpType::OPEND) {
        auto textSize = ImGui::CalcTextSize(std::format("{:.2f}", node->GetValue()).c_str());
        if (textSize.x > radius)
            radius = textSize.x / 2;
        draw_list->AddCircleFilled(position, radius, ::VAL_NODE_COL);
        draw_list->AddText(ImVec2(position.x - textSize.x / 2, position.y - textSize.y / 2), ::TXT_COL, std::format("{:.1f}", node->GetValue()).c_str());

    }
    else {
        draw_list->AddCircleFilled(position, radius, ::OP_NODE_COL);
        ImGui::PushFont(NULL, 42);
        auto textSize = ImGui::CalcTextSize(OpTypeToString(node->GetType()).c_str());
        draw_list->AddText(ImVec2(position.x - textSize.x / 2, position.y - textSize.y / 2), ::TXT_COL, OpTypeToString(node->GetType()).c_str());
        ImGui::PopFont();
    }
}

void RenderBinaryTree(Expression* exp) {

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    auto curr_cursor_pos = ImGui::GetCursorScreenPos();
    ImVec2 startPosition = ImVec2(Imeasy::width / 2 + curr_cursor_pos.x, 40 + curr_cursor_pos.y);
    if (exp == NULL || (exp->pLhs == NULL && exp->pRhs == NULL)) {
        ImGui::PushFont(NULL, 102);
        draw_list->AddText(ImVec2(startPosition.x, startPosition.y * 2), IM_COL32(255, 255, 255, 100), "?");
        ImGui::PopFont();
        return;
    }
    int cWidth = CalculateSubTreeWidth(exp);
    ImGui::Dummy(ImVec2(cWidth * 10, 0));
    DrawBinaryTree(draw_list, exp, startPosition, (120 + Imeasy::width / 20) * tree_zoom_scale, (70 + Imeasy::height / 20) * tree_zoom_scale);
    ImGuiIO& io = ImGui::GetIO();
    if (ImGui::IsWindowHovered() && io.KeyCtrl && io.MouseWheel != 0.0f) {
        tree_zoom_scale += io.MouseWheel * TREE_ZOOM_SCALE_INC_SPEED;

        if (tree_zoom_scale > TREE_MAX_ZOOM_SCALE)
            tree_zoom_scale = TREE_MAX_ZOOM_SCALE;
        if (tree_zoom_scale < TREE_MIN_ZOOM_SCALE)
            tree_zoom_scale = TREE_MIN_ZOOM_SCALE;
    }

}