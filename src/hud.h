#ifndef HUD_H
#define HUD_H

#include <stdbool.h>
#include "camera.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration to avoid including GLFW headers here
typedef struct GLFWwindow GLFWwindow;

// Initialize Dear ImGui for a GLFW + OpenGL3 context
void hud_init(GLFWwindow* window);

// Begin a new ImGui frame (call once per frame before updating UI)
void hud_new_frame(void);

// Build/update the HUD UI. This does not render; it only updates state based on UI.
// - fps: current frames per second
// - numActive: current number of active particles
// - clearRequested: set to true when the user presses the Clear button
// - camera/cameraRadius/autoOrbit: can be modified by UI buttons to change view
void hud_update(float fps, int numActive, bool* clearRequested,
                Camera* camera, float* cameraRadius, bool* autoOrbit);

// Render the HUD (call once per frame after your 3D rendering, before buffer swap)
void hud_render(void);

// Shutdown and free ImGui resources
void hud_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // HUD_H
