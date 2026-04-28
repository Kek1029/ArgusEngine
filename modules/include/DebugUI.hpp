
#pragma once
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <vector>


// TODO: выколите мне глаза, пожалуйста

namespace ArgusEngine {
    class DebugUI {
    public:
        static void init(GLFWwindow* window) {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGui_ImplGlfw_InitForOpenGL(window, true);
            ImGui_ImplOpenGL3_Init("#version 430");
            ImGui::StyleColorsDark();
        }

        static void begin() {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
        }

        static void render(float dt, uint32_t totalEntities, uint32_t visibleEntities) {
            ImGui::SetNextWindowPos({10, 10}, ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize({300, 200}, ImGuiCond_FirstUseEver);
            ImGui::Begin("Perfomance", nullptr, ImGuiWindowFlags_NoSavedSettings);

            static float frameTimes[120] = {0};
            static int offset = 0;
            frameTimes[offset] = dt * 1000.0f;
            offset = (offset + 1) % 120;

            float avg = 0;
            for(int n=0; n<120; n++) avg += frameTimes[n];
            avg /= 120.0f;

            ImGui::Text("FPS: %.1f (%.2f ms)", 1.0f / dt, dt * 1000.0f);
            ImGui::PlotLines("##FrameTime", frameTimes, 120, offset, "ms", 0.0f, 33.0f, ImVec2(0, 80));
            //if (offset == 0) {
            //    std::cout << frameTimes[0] << " Visible Entities: " << visibleEntities << std::endl;
            //}
            ImGui::Separator();
            ImGui::Text("Total Entities: %u", totalEntities);
            ImGui::Text("Visible (GPU): %u", visibleEntities);

            ImGui::End();
        }

        static void draw() {
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        static void shutdown() {
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
        }
    };
}