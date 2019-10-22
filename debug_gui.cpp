#include "debug_gui.hpp"

#include <imgui.h>

#include <examples/imgui_impl_glfw.h>
#include <examples/imgui_impl_opengl3.h>

#include <windows.h>

bool DebugGUI::openFileDialog(std::string &path, const char *const filter) {
    OPENFILENAMEA ofn;
    char szFile[MAX_PATH + 1] = "";
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST;

    char cwd[MAX_PATH];
    GetCurrentDirectory(sizeof(cwd), cwd);
    bool result = GetOpenFileName(&ofn);
    SetCurrentDirectory(cwd);

    if (result) {
        path.assign(szFile);
    }

    return result;
}

bool DebugGUI::saveFileDialog(std::string &path, const char *const filter,
                              const char *const defExt) {
    OPENFILENAMEA ofn;
    char szFile[MAX_PATH + 1] = "";
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = szFile;
    ofn.lpstrDefExt = defExt;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT;

    char cwd[MAX_PATH];
    GetCurrentDirectory(sizeof(cwd), cwd);
    bool result = GetSaveFileName(&ofn);
    SetCurrentDirectory(cwd);

    if (result) {
        path.assign(szFile);
    }

    return result;
}

DebugGUI::DebugGUI() {}

void DebugGUI::setup(GLFWwindow *window,
                     std::shared_ptr<const ImageBuffer> image) {
    this->image = image;

    ImGui::SetCurrentContext(ImGui::CreateContext());
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core\n");
}

void DebugGUI::beginFrame() {
    static bool showImGuiDemoWindow = false;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (showImGuiDemoWindow) {
        ImGui::ShowDemoWindow(&showImGuiDemoWindow);
    }

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open", "Ctrl+O")) {
                this->onOpenGLB();
            }
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                this->onSaveImage();
            }
            if (ImGui::MenuItem("Quit", "Alt+F4")) {
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void DebugGUI::onSaveImage() {
    saveFileDialog(savingImagePath, "Image File {*.png}", "png");
}

void DebugGUI::onOpenGLB() {
    openFileDialog(openingGLBPath, "GLB File {*.glb}");
}

void DebugGUI::renderFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

std::string DebugGUI::pullOpeningGLBPath() {
    if (!openingGLBPath.empty()) {
        auto result = openingGLBPath;

        openingGLBPath.clear();

        return result;
    }

    return openingGLBPath;
}

std::string DebugGUI::pullSavingImagePath() {
    if (!savingImagePath.empty()) {
        auto result = savingImagePath;

        savingImagePath.clear();

        return result;
    }

    return savingImagePath;
}
