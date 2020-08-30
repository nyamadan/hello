#include <windows.h>

#include <imgui.h>
#include <stb_image_write.h>
#include <numeric>
#include <sstream>

#include <examples/imgui_impl_glfw.h>
#include <examples/imgui_impl_opengl3.h>

#include "debug_gui.hpp"

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

RenderingMode DebugGUI::getRenderingMode() const { return this->renderingMode; }

bool DebugGUI::getEnableSuperSampling() const {
    return this->enableSuperSampling;
}

void DebugGUI::setup(GLFWwindow *window) {
    ImGui::SetCurrentContext(ImGui::CreateContext());
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core\n");
}

std::list<DebugGuiCommand> DebugGUI::beginFrame(const RayTracer &raytracer,
                                                ConstantPModel model) {
    std::list<DebugGuiCommand> commands;

    static bool showImGuiDemoWindow = false;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (showImGuiDemoWindow) {
        ImGui::ShowDemoWindow(&showImGuiDemoWindow);
    }

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open Model", "Ctrl+O")) {
                glbPath = "";
                openFileDialog(glbPath,
                               "GLB File (*.glb)\0*.glb\0"
                               "GLTF File (*.gltf)\0*.gltf\0"
                               "OBJ File (*.obj)\0*.obj\0"
                               "All Files (*.*)\0*.*\0\0");
                if (!glbPath.empty()) {
                    currAnimIndex = 0;
                    animTime = 0.0f;
                    commands.push_back(DebugLoadModel);
                }
            }

            if (ImGui::MenuItem("Open Lua")) {
                luaPath = "";
                openFileDialog(luaPath,
                               "Lua File (*.lua)\0*.lua\0"
                               "All Files (*.*)\0*.*\0\0");
                if (!luaPath.empty()) {
                    commands.push_back(DebugOpenLua);
                }
            }

            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                const auto image = raytracer.getImage();
                std::string path;
                saveFileDialog(path, "Image File (*.png)\0*.png\0\0", "png");
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

    if (!this->isActive()) {
        const auto w = raytracer.getImage().getWidth();
        const auto h = raytracer.getImage().getHeight();
        double xpos, ypos;
        glfwGetCursorPos(glfwGetCurrentContext(), &xpos, &ypos);
        const auto x =
            static_cast<int32_t>(glm::clamp(xpos / bufferScale, 0.0, w - 1.0));
        const auto y = static_cast<int32_t>(
            glm::clamp(h - (ypos / bufferScale), 0.0, h - 1.0));
        const auto &color = raytracer.getImage().GetReadonlyBuffer()[y * w + x];

        ImGui::SetTooltip("%.2f, %.2f, %.2f", color.r, color.g, color.b);
    }

    ImGui::Begin(
        "Hello Embree", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);
    {
        deltaTimes[deltaTimesOffset] = ImGui::GetIO().DeltaTime;

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

        ImGui::Combo("CameraControl", reinterpret_cast<int32_t *>(&cameraMode),
                     CameraModeNames, IM_ARRAYSIZE(CameraModeNames));

        ImGui::Separator();

        if (ImGui::Combo("Mode", reinterpret_cast<int32_t *>(&renderingMode),
                         RenderingModeName, IM_ARRAYSIZE(RenderingModeName))) {
            if (renderingMode == PATHTRACING) {
                ImGui::OpenPopup("Render");
            }

            commands.push_back(DebugUpdate);
        }

        ImGui::Separator();

        if (ImGui::InputInt("BufferScale", &bufferScale)) {
            bufferScale = std::clamp<int32_t>(bufferScale, 1, 8);

            commands.push_back(DebugResize);
        }

        if (ImGui::InputInt("Samples", &samples)) {
            samples = std::max<int32_t>(samples, 1);
            commands.push_back(DebugUpdate);
        }

        if (ImGui::InputFloat("LensRadius", &lensRadius, 0.1f, 1.0f)) {
            lensRadius = std::max<float>(lensRadius, 0.0f);
            commands.push_back(DebugUpdate);
        }

        if (ImGui::InputFloat("FocusDistance", &focusDistance, 1.0f, 10.0f)) {
            focusDistance = std::max<float>(focusDistance, 0.0f);
            commands.push_back(DebugUpdate);
        }

        if (ImGui::Checkbox("isEquirectangular", &isEquirectangular)) {
            commands.push_back(DebugResize);
        }

        if (ImGui::Checkbox("SuperSampling", &enableSuperSampling)) {
            commands.push_back(DebugUpdate);
        }

        ImGui::Separator();

        bool animatedModel =
            model.get() != nullptr && model->getAnimations().size() > 0;

        if (animatedModel) {
            const auto &animations = model->getAnimations();
            const auto len = animations.size();
            auto labels = std::unique_ptr<std::string[]>(new std::string[len]);
            auto items = std::unique_ptr<const char *[]>(new const char *[len]);
            for (auto i = 0; i < len; i++) {
                std::stringstream ss;
                ss << i << " : " << animations[i]->getName();
                labels[i] = ss.str();
                items[i] = labels[i].c_str();
            }

            if (ImGui::Combo("Animations", &currAnimIndex, items.get(), len)) {
                commands.push_back(DebugGeometryUpdate);
            }

            auto anim = animations[currAnimIndex];

            auto min = anim->getTimelineMin();
            auto max = anim->getTimelineMax();

            if (ImGui::SliderFloat("Time", &animTime, min, max)) {
                commands.push_back(DebugGeometryUpdate);
            }

            ImGui::Separator();
        }

        ImGui::Separator();

        if (ImGui::Button("Render")) {
            isMovie = false;

            commands.push_back(DebugUpdate);
            isRendering = true;

            ImGui::OpenPopup("Render");
        }

        if (animatedModel) {
            ImGui::SameLine();
            if (ImGui::Button("Render as Movie")) {
                y4mPath = "";
                saveFileDialog(y4mPath, "y4m file (*.y4m)\0*.y4m\0\0", "y4m");
                if (!y4mPath.empty()) {
                    commands.push_back(DebugSaveMovie);
                    isMovie = true;

                    commands.push_back(DebugUpdate);
                    isRendering = true;

                    currFrame = 0;
                    animTime = 0;

                    ImGui::OpenPopup("Render as Movie");
                }
            }
        }

        if (ImGui::BeginPopupModal("Render", nullptr,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Samples");
            ImGui::ProgressBar(static_cast<float>(raytracer.getSamples()) /
                                   raytracer.getMaxSamples(),
                               ImVec2(240, 16));
            ImGui::Separator();
            if (ImGui::Button("Cancel")) {
                isRendering = false;
            }
            if (!isRendering) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        } else if (this->getIsRendering() && !this->getIsMovie()) {
            ImGui::OpenPopup("Render");
        }

        if (ImGui::BeginPopupModal("Render as Movie", nullptr,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            if (animatedModel) {
                ImGui::Text("Animations");
                ImGui::ProgressBar(
                    animTime /
                        model->getAnimations()[currAnimIndex]->getTimelineMax(),
                    ImVec2(240, 16));
            }
            ImGui::Text("Samples");
            ImGui::ProgressBar(static_cast<float>(raytracer.getSamples()) /
                                   raytracer.getMaxSamples(),
                               ImVec2(240, 16));
            ImGui::Separator();
            if (ImGui::Button("Cancel")) {
                commands.push_back(DebugCancelSaveMovie);
                isMovie = false;

                isRendering = false;
            }
            if (!isRendering) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        } else if (this->getIsRendering() && this->getIsMovie()) {
            ImGui::OpenPopup("Render as Movie");
        }
    }

    ImGui::End();

    ImGui::Render();

    commands.unique();

    return commands;
}

bool DebugGUI::nextFrame(ConstantPAnimation anim) {
    auto min = anim->getTimelineMin();
    auto max = anim->getTimelineMax();

    animTime = static_cast<float>(++currFrame * 1001.0 / 30000.0);
    return animTime <= max;
}

void DebugGUI::renderFrame() const {
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

const std::string &DebugGUI::getGlbPath() const { return glbPath; }

const std::string &DebugGUI::getY4mPath() const { return y4mPath; }

const std::string &DebugGUI::getLuaPath() const { return luaPath; }

CameraMode DebugGUI::getCameraMode() const { return cameraMode; }

int32_t DebugGUI::getBufferScale() const { return bufferScale; }

int32_t DebugGUI::getSamples() const { return samples; }

void DebugGUI::setIsRendering(bool v) { isRendering = v; }

bool DebugGUI::getIsRendering() const { return isRendering; }

bool DebugGUI::getIsEquirectangular() const { return isEquirectangular; }

float DebugGUI::getFocusDistance() const { return focusDistance; }

float DebugGUI::getLensRadius() const { return lensRadius; }

int32_t DebugGUI::getAnimIndex() const { return currAnimIndex; }

float DebugGUI::getAnimTime() const { return animTime; }

bool DebugGUI::getIsMovie() const { return isMovie; }

bool DebugGUI::isActive() const { return ImGui::GetIO().WantCaptureMouse; }
