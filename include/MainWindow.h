#include <BinaryTree.h>
#include <PParser.h>
#include <iostream>
#include <string>

extern char buffer[256];
extern char b_cmp[256];
extern double answer;
int MainWindow();
void RenderMainWindow();
void HideConsoleWindow();
void OnExpressionTextChanged(char* text);
void OnButtonClicked();
