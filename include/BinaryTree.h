#include <imgui_impl_easy.h>
#include <PParser.h>
#include <math.h>
#include <format>
#pragma once

#define RADIUS 35
#define CIRCLE_MIN_RADIUS 15
extern float tree_zoom_scale;
const float TREE_MIN_ZOOM_SCALE = 0.01f;
const float TREE_MAX_ZOOM_SCALE = 5.0f;
const float TREE_ZOOM_SCALE_INC_SPEED = 0.1f;

const auto OP_NODE_COL = IM_COL32(0,0 , 255, 200);
const auto VAL_NODE_COL = IM_COL32(255, 0, 0, 255);
const auto TXT_COL = IM_COL32(255, 255, 255, 255);

int CalculateSubTreeWidth(Expression* node);
void DrawBinaryTree(ImDrawList* draw_list, Expression* node, ImVec2 position, float X, float Y);
void RenderBinaryTree(Expression* exp);
