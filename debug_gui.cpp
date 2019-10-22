#include "debug_gui.hpp"

#include <imgui.h>

#include <examples/imgui_impl_glfw.h>
#include <examples/imgui_impl_opengl3.h>

#include <stb_image_write.h>

#include <nfd.h>

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
    nfdchar_t *outPath = nullptr;
    const auto result = NFD_SaveDialog("png", nullptr, &outPath);
    if (result == NFD_OKAY) {
        stbi_flip_vertically_on_write(true);
        stbi_write_png(outPath, image->getWidth(), image->getHeight(), 3, image->GetReadonlyBuffer(), 3 * image->getWidth());
    }
}

void DebugGUI::renderFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}