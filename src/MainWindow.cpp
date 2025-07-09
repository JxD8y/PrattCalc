#include <MainWindow.h>


char buffer[256] = {};
char b_cmp[256] = {};
double answer = 0.0f;
static Expression* expr = NULL;

int MainWindow() {
    ::HideConsoleWindow();
    Imeasy::AddRenderingObject(RenderMainWindow);
    Imeasy::StartRendering("PrattCalc - Jxd8y : v0.1", 600, 800);
    return 0;
}
void OnExpressionTextChanged(char* text) {
    try {
        PParser p(buffer);
        expr = p.Parse();
        answer = SolveExpression(expr);
    }
    catch (exception& e) {
        cout << "Exception caught: " << e.what() << endl;
    }
}

void HideConsoleWindow() {
    HWND console = GetConsoleWindow();
    ShowWindow(console, SW_HIDE);
}
void RenderMainWindow() {
    ImGui::Begin("PrattCalc", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_HorizontalScrollbar);
    ImVec2 size(Imeasy::width, Imeasy::height);
    ImGui::SetWindowSize(size, ImGuiCond_Always);
    ImGui::SetWindowPos(ImVec2(0, 0));
    ImGui::Text("Expression: ");
    float av_x = ImGui::GetContentRegionAvail().x;
    ImGui::SetNextItemWidth(av_x - 20);
    strcpy_s(b_cmp, sizeof(b_cmp), buffer);
    ImGui::InputText(" ", buffer, sizeof(buffer));

    if (strcmp(b_cmp, buffer) && strcmp(buffer, ""))
        OnExpressionTextChanged(buffer);

    ImGui::PushFont(NULL, 35);
    ImGui::Text("Calculated answer: %.2f",answer);
    ImGui::Text("Rendered tree: ");
    ImGui::PopFont();

    RenderBinaryTree(expr);
    ImGui::End();
}