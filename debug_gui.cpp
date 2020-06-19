#include "debug_gui.hpp"
#include <imgui.h>
#include <stb_image_write.h>
#include <numeric>

#include <examples/imgui_impl_glfw.h>
#include <examples/imgui_impl_opengl3.h>

#include <windows.h>

bool DebugGUI::openFileDialog(std::string &path, const char *const filter) {
    OPENFILENAMEA ofn;
    char szFile[MAX_PATH + 1] = "";
    memset(&ofn, 0, sizeof(ofn));
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

RenderingMode DebugGUI::getRenderingMode() const {
    return this->renderingMode;
}

bool DebugGUI::getEnableSuperSampling() const {
    return this->enableSuperSampling;
}

void DebugGUI::setup(GLFWwindow *window) {
    ImGui::SetCurrentContext(ImGui::CreateContext());
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core\n");
}

void DebugGUI::beginFrame(const RayTracer &raytracer, bool &needUpdate, bool &needResize, bool &needRestart) {
    static bool showImGuiDemoWindow = true;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (showImGuiDemoWindow) {
        ImGui::ShowDemoWindow(&showImGuiDemoWindow);
    }

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open", "Ctrl+O")) {
                openFileDialog(glbPath, "GLB File {*.glb}");
                if(!glbPath.empty()) {
                    needRestart = true;
                }
            }

            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                const auto image = raytracer.getImage();
                std::string path;
                saveFileDialog(path, "Image File {*.png}", "png");
                if (!path.empty()) {
                    stbi_flip_vertically_on_write(true);
                    stbi_write_png(path.c_str(), image.getWidth(),
                                   image.getHeight(), image.getChannels(),
                                   image.GetTextureBuffer(),
                                   image.getChannels() * image.getWidth());
                }
            }

            if (ImGui::MenuItem("Quit", "Alt+F4")) {
                glfwSetWindowShouldClose(glfwGetCurrentContext(), GLFW_TRUE);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    ImGui::Begin(
        "Hello Embree", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);
    {
        deltaTimes[deltaTimesOffset] = ImGui::GetIO().DeltaTime;

        {
            ImGui::LabelText("mode", "%s",
                             RenderingModeName[raytracer.getRenderingMode()]);
        }

        {
            ImGui::LabelText("samples", "%d / %d", raytracer.getSamples(),
                             raytracer.getMaxSamples());
        }

        auto average =
            std::reduce(deltaTimes.cbegin(), deltaTimes.cend(), 0.0f) /
            deltaTimes.size();
        deltaTimesOffset = (deltaTimesOffset + 1) % deltaTimes.size();
        std::array<char, 32> overlay{};
        std::snprintf(&overlay[0], overlay.size(), "FPS: %.2f", 1.0f / average);
        ImGui::PlotHistogram(
            "ms", deltaTimes.data(), static_cast<int32_t>(deltaTimes.size()),
            deltaTimesOffset, overlay.data(), .0f, .5f, ImVec2(0, 80.0f));

        ImGui::Separator();

        if (ImGui::Combo("Mode", reinterpret_cast<int32_t *>(&renderingMode),
                         RenderingModeName, IM_ARRAYSIZE(RenderingModeName))) {
            needUpdate = true;
        }

        if (ImGui::InputInt("BufferScale", &bufferScale)) {
            bufferScale = std::clamp<int32_t>(bufferScale, 1, 8);
            needResize = true;
        }

        if (ImGui::InputInt("Samples", &samples)) {
            samples = std::max<int32_t>(samples, 1);
            needUpdate = true;
        }

        if (ImGui::Checkbox("SuperSampling", &enableSuperSampling)) {
            needUpdate = true;
        }
    }

    ImGui::End();

    ImGui::Render();
}

void DebugGUI::renderFrame() const {
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

std::string DebugGUI::getGlbPath() const {
    return glbPath;
}

int32_t DebugGUI::getBufferScale() const {
    return bufferScale;
}

int32_t DebugGUI::getSamples() const {
    return samples;
}