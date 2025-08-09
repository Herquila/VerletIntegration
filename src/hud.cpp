#include "hud.h"

// Ensure GLEW is the GL loader and include it before any other GL headers
#ifndef IMGUI_IMPL_OPENGL_LOADER_GLEW
#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#endif
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Dear ImGui
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

void hud_init(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void hud_new_frame(void)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

static void set_view_x(Camera* camera, float radius)
{
    if (!camera) return;
    camera->pitch = 0.0f;
    camera->yaw = 180.0f;
    camera->position[0] = radius; camera->position[1] = 0.0f; camera->position[2] = 0.0f;
}

static void set_view_y(Camera* camera, float radius)
{
    if (!camera) return;
    camera->pitch = -90.0f;
    camera->position[0] = 0.0f; camera->position[1] = radius; camera->position[2] = 0.0f;
}

static void set_view_z(Camera* camera, float radius)
{
    if (!camera) return;
    camera->pitch = 0.0f;
    camera->yaw = -90.0f;
    camera->position[0] = 0.0f; camera->position[1] = 0.0f; camera->position[2] = radius;
}

void hud_update(float fps, int numActive, bool* clearRequested,
                Camera* camera, float* cameraRadius, bool* autoOrbit)
{
    if (clearRequested) *clearRequested = false;

    ImGui::Begin("HUD");
    ImGui::Text("FPS: %.1f", fps);
    ImGui::Text("Balls: %d", numActive);
    if (clearRequested && ImGui::Button("Clear (accel+vel)")) {
        *clearRequested = true;
    }

    if (autoOrbit && ImGui::Checkbox("Auto Orbit", autoOrbit)) {
        // toggled via checkbox
    }
    if (cameraRadius) {
        ImGui::SliderFloat("Camera Radius", cameraRadius, 5.0f, 100.0f, "%.1f");
    }

    ImGui::Separator();
    ImGui::Text("Views");
    if (ImGui::Button("View X")) { if (autoOrbit) *autoOrbit = false; set_view_x(camera, cameraRadius ? *cameraRadius : 24.0f); }
    ImGui::SameLine();
    if (ImGui::Button("View Y")) { if (autoOrbit) *autoOrbit = false; set_view_y(camera, cameraRadius ? *cameraRadius : 24.0f); }
    ImGui::SameLine();
    if (ImGui::Button("View Z")) { if (autoOrbit) *autoOrbit = false; set_view_z(camera, cameraRadius ? *cameraRadius : 24.0f); }
    ImGui::SameLine();
    if (ImGui::Button("Orbit")) { if (autoOrbit) *autoOrbit = true; }

    ImGui::End();
}

void hud_render(void)
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void hud_shutdown(void)
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
