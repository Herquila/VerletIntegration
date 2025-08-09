#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <GL/glew.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "shader.h"
#include "graphics.h"
#include "peripheral.h"
#include "camera.h"
#include "model.h"
#include "verlet.h"
#include "hud.h"

// Preprocessor constants
#define ANIMATION_TIME 90.0f // Frames
#define ADDITION_SPEED 10
#define TARGET_FPS 60
#define NUM_SUBSTEPS 8
#define MAX_SPAWNS_PER_FRAME 1 // Limit how many particles can be spawned per render frame

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void cursor_enter_callback(GLFWwindow* window, int entered);
void processInput(GLFWwindow* window);
void updateCamera(GLFWwindow* window, Mouse* mouse, Camera* camera);
void instantiateVerlets(VerletObject* objects, int size);
void spawnVerlet(VerletObject* objects, int* numActive, int maxInstances, mfloat_t* position, mfloat_t* velocity, ParticleColor color, mfloat_t radius);

// Settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// Global variables (replacing soon with State struct)
bool cursorEntered = false;
Camera* camera;
float cameraRadius = 24.0f;
int totalFrames = 0;
// Enable/disable automatic orbiting of the camera
bool autoOrbit = true;

const char* vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\0";

const char* fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
    "}\0";

int main() {
    
    GLFWwindow* window;
    
    // Initialize GLFW
    if (!glfwInit()) {
        return -1;  // Initialization failed
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);  // Ensure depth buffer is available

    #ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif
    
    // Create a windowed mode window and its OpenGL context
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Verlet Integration", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;  // Window or OpenGL context creation failed
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    /* Set up a callback function for when the window is resized */
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSwapInterval(1);
    
    /* Callback function for mouse enter/exit */
    glfwSetCursorEnterCallback(window, cursor_enter_callback);

    // Initialize GLEW
    glewExperimental = GL_TRUE;  // Ensures GLEW uses more modern techniques for managing OpenGL functionality
    if (glewInit() != GLEW_OK) {
        return -1;  // GLEW initialization failed
    }

    /* OpenGL Settings */
    glClearColor(0.1, 0.1, 0.1, 1.0);
    glClearStencil(0);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPointSize(3.0);

    // Initialize HUD (Dear ImGui)
    hud_init(window);

    /* Models & Shaders */
    unsigned int phongShader = createShader("shaders/phong_vertex.glsl", "shaders/phong_fragment.glsl");
    unsigned int instanceShader = createShader("shaders/instance_vertex.glsl", "shaders/instance_fragment.glsl");
    unsigned int baseShader = createShader("shaders/base_vertex.glsl", "shaders/base_fragment.glsl");

    Mesh* mesh = createMesh("models/sphere.obj", true);
    //Mesh* cubeMesh = createMesh("models/cube.obj", false);
    
    // Container
    mfloat_t containerPosition[VEC3_SIZE] = { 0, 0, 0 };
    mfloat_t rotation[VEC3_SIZE] = { 0, 0, 0 };
    
    VerletObject* verlets = malloc(sizeof(VerletObject) * MAX_INSTANCES);
    //for (int i = 0; i < MAX_INSTANCES; ++i) {
    //    verlets[i].visible = false;
    //   }
    instantiateVerlets(verlets, MAX_INSTANCES);
    int numActive = 0;
    
    mfloat_t view[MAT4_SIZE];
    camera = createCamera((mfloat_t[]) { 0, 0, cameraRadius });

    Mouse* mouse = createMouse();

    float dt = 0.000001f;
    float lastFrameTime = (float)glfwGetTime();

    char title[100] = "";

    srand(time(NULL));

    // Main loop

    while (!glfwWindowShouldClose(window)) {
        /* Input */
        updateMouse(window, mouse);
        processInput(window);
        int visibleCount = 0; // Count only visible objects

        // Start HUD frame and update controls
        hud_new_frame();
        bool clearFromHUD = false;
        float fps_for_ui = (dt > 1e-6f) ? (1.0f / dt) : (float)TARGET_FPS;
        hud_update(fps_for_ui, numActive, &clearFromHUD, camera, &cameraRadius, &autoOrbit);

        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
            addForce(verlets, numActive, (mfloat_t[]) { 0, 3, 0 }, -30.0f * NUM_SUBSTEPS);
        }

        /* Camera */
        updateCamera(window, mouse, camera);
        createViewMatrix(view, camera);

        /* Shader Uniforms */
        glUseProgram(phongShader);
        glUniformMatrix4fv(glGetUniformLocation(phongShader, "view"),
            1, GL_FALSE, view);
        glUseProgram(0);
        glUseProgram(baseShader);
        glUniformMatrix4fv(glGetUniformLocation(baseShader, "view"),
            1, GL_FALSE, view);
        glUseProgram(0);
        glUseProgram(instanceShader);
        glUniformMatrix4fv(glGetUniformLocation(instanceShader, "view"),
            1, GL_FALSE, view);
        glUseProgram(0);
        
        
        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        
        int spawnsThisFrame = 0; // Limit spawns per frame
        
        if (1.0 / dt >= TARGET_FPS - 5 && glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS && numActive < MAX_INSTANCES) {
            numActive += ADDITION_SPEED;
        }
        // Auto-spawn one red ball once per second at the top of the container
        static double lastAutoSpawn = 0.0;
        double now = glfwGetTime();
        if (spawnsThisFrame < MAX_SPAWNS_PER_FRAME && (now - lastAutoSpawn) >= 1.0) {
            // Spawn at the top (north pole) of the container sphere, offset by particle radius to keep it inside
            mfloat_t pos[VEC3_SIZE] = { containerPosition[0], containerPosition[1] + CONTAINER_RADIUS - VERLET_RADIUS, containerPosition[2] };
            mfloat_t vel[VEC3_SIZE] = {0, 0, 0}; // initial velocity
            ParticleColor color = RED; // or random/color cycling
            spawnVerlet(verlets, &numActive, MAX_INSTANCES, pos, vel, color, VERLET_RADIUS);
            spawnsThisFrame++;
            lastAutoSpawn = now;
        }
        if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS && spawnsThisFrame < MAX_SPAWNS_PER_FRAME) {
            // Spawn at the top (north pole) of the container sphere, offset by particle radius to keep it inside
            mfloat_t pos[VEC3_SIZE] = { containerPosition[0], containerPosition[1] + CONTAINER_RADIUS - VERLET_RADIUS, containerPosition[2] };
            mfloat_t vel[VEC3_SIZE] = {0, 0, 0}; // initial velocity
            ParticleColor color = RED; // or random/color cycling
            spawnVerlet(verlets, &numActive, MAX_INSTANCES, pos, vel, color, VERLET_RADIUS);
            spawnsThisFrame++;
        }
        if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS && spawnsThisFrame < MAX_SPAWNS_PER_FRAME) {
            mfloat_t pos[VEC3_SIZE] = {0, 0, 0}; // spawn at origin or any position
            mfloat_t vel[VEC3_SIZE] = {0, 0, 0}; // initial velocity
            ParticleColor color = GREEN; // or random/color cycling
            spawnVerlet(verlets, &numActive, MAX_INSTANCES, pos, vel, color, VERLET_RADIUS);
            spawnsThisFrame++;
        }
        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS && spawnsThisFrame < MAX_SPAWNS_PER_FRAME) {
            mfloat_t pos[VEC3_SIZE] = {0, 0, 0}; // spawn at origin or any position
            mfloat_t vel[VEC3_SIZE] = {0, 0, 0}; // initial velocity
            ParticleColor color = BLUE; // or random/color cycling
            spawnVerlet(verlets, &numActive, MAX_INSTANCES, pos, vel, color, VERLET_RADIUS);
            spawnsThisFrame++;
        }
        if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && spawnsThisFrame < MAX_SPAWNS_PER_FRAME) {
            mfloat_t pos[VEC3_SIZE] = {0, 0, 0}; // spawn at origin or any position
            mfloat_t vel[VEC3_SIZE] = {0, 0, 0}; // initial velocity
            ParticleColor color = WHITE; // or random/color cycling
            mfloat_t radius = VERLET_RADIUS; // or any desired radius
            spawnVerlet(verlets, &numActive, MAX_INSTANCES, pos, vel, color, radius);
            spawnsThisFrame++;
        } 

        //printf("Hello visiblecount: %d\n ", visibleCount); 

        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            containerPosition[0] -= 0.05f;
        }
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            containerPosition[0] += 0.05f;
        }
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            containerPosition[1] -= 0.05f;
        }
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
            containerPosition[1] += 0.05f;
        }

        float sub_dt = dt / NUM_SUBSTEPS;
        // Press 'C' or HUD Clear to clear all accelerations this frame
        bool clearAccels = (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) || clearFromHUD;
        for (int i = 0; i < NUM_SUBSTEPS; i++) {
            applyForces(verlets, numActive);
            if (clearAccels) {
                for (int j = 0; j < numActive; ++j) {
                    vec3_zero(verlets[j].acceleration);
                }
            }
            // applyCollisions(verlets, numActive);
            applyGridCollisions(verlets, numActive);
            applyConstraints(verlets, numActive, containerPosition);
            // If clearing, also zero velocity by making previous == current before integration
            if (clearAccels) {
                for (int j = 0; j < numActive; ++j) {
                    vec3_assign(verlets[j].previous, verlets[j].current);
                }
            }
            updatePositions(verlets, numActive, sub_dt);
        }

         // Assuming numActive is the number of active particles
        float verletPositions[numActive * VEC3_SIZE];
        float verletVelocities[numActive];
        float verletColors[numActive * VEC3_SIZE]; // Array to store color data

        int posPointer = 0;
        int velPointer = 0;
        int colorPointer = 0;
        for (int i = 0; i < numActive; i++) {
            VerletObject obj = verlets[i];
            if (!obj.visible) continue; // Only process visible objects
            // Position data
            verletPositions[posPointer++] = obj.current[0];
            verletPositions[posPointer++] = obj.current[1];
            verletPositions[posPointer++] = obj.current[2];

            // Velocity data
            float vel = vec3_distance(obj.current, obj.previous) * 10;
            verletVelocities[velPointer++] = vel;

            // Color data
            verletColors[colorPointer++] = obj.colorVector[0];
            verletColors[colorPointer++] = obj.colorVector[1];
            verletColors[colorPointer++] = obj.colorVector[2];
            visibleCount++; // Count only visible objects
        }
        
        if (totalFrames % 60 == 0) {
            sprintf(title, "FPS : %-4.0f | Balls : %-10d | Inactive : %-10d", 1.0 / dt,numActive, visibleCount);
            glfwSetWindowTitle(window, title);
        }
        // Update position VBO
        glBindBuffer(GL_ARRAY_BUFFER, mesh->positionVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * visibleCount * VEC3_SIZE, verletPositions);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Update velocity VBO
        glBindBuffer(GL_ARRAY_BUFFER, mesh->velocityVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * visibleCount, verletVelocities);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Update color VBO
        glBindBuffer(GL_ARRAY_BUFFER, mesh->colorVBO); // Assume you have created a VBO for colors
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * visibleCount * VEC3_SIZE, verletColors);
        glBindBuffer(GL_ARRAY_BUFFER, 0);



        /* Draw instanced verlet objects */
        drawInstanced(mesh, instanceShader, GL_TRIANGLES, visibleCount, verlets[0].radius);

        /* Container */
        drawMesh(mesh, baseShader, GL_POINTS, containerPosition, rotation, CONTAINER_RADIUS * 1.02);

        // Render HUD on top
        hud_render();

        // Swap front and back buffers
        glfwSwapBuffers(window);
        
        // Poll for and process events
        glfwPollEvents();
        
        /* Timing */
        dt = (float)glfwGetTime() - lastFrameTime;
        while (dt < 1.0f / TARGET_FPS) {
            dt = (float)glfwGetTime() - lastFrameTime;
        }
        lastFrameTime = (float)glfwGetTime();
        totalFrames++;
        //if (numActive > 0) {
        //    VerletObject* obj = &verlets[0];
        //    float vx = (obj->current[0] - obj->previous[0]) / dt;
        //    float vy = (obj->current[1] - obj->previous[1]) / dt;
        //    float vz = (obj->current[2] - obj->previous[2]) / dt;
        //    printf("Particle 0 velocity: (%f, %f, %f)\n", vx, vy, vz);
        //}
    }
    // Shutdown HUD
    hud_shutdown();
    free(verlets);
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Axis-aligned camera views
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        autoOrbit = false;
        camera->pitch = 0.0f;      // level
        camera->yaw = 180.0f;      // look toward -X from +X
        vec3(camera->position, cameraRadius, 0.0f, 0.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
        autoOrbit = false;
        camera->pitch = -90.0f;    // look down along -Y from +Y
        // yaw is irrelevant when pitch is +/-90
        vec3(camera->position, 0.0f, cameraRadius, 0.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        autoOrbit = false;
        camera->pitch = 0.0f;      // level
        camera->yaw = -90.0f;      // look toward -Z from +Z
        vec3(camera->position, 0.0f, 0.0f, cameraRadius);
    }
    // Resume orbit
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
        autoOrbit = true;
    }
}

void updateCamera(GLFWwindow* window, Mouse* mouse, Camera* camera)
{

    float speed = 0.08f;
    mfloat_t temp[VEC3_SIZE];

    float universalAngle = totalFrames / 4.0f;
    if (autoOrbit) {
        vec3(camera->position, MCOS(MRADIANS(universalAngle)) * cameraRadius, camera->position[1], MSIN(MRADIANS(universalAngle)) * cameraRadius);
        camera->yaw = universalAngle + 180.0f;
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        vec3_add(camera->position, camera->position, vec3_multiply_f(temp, camera->up, speed));
        camera->pitch -= 0.22f;
        cameraRadius -= 0.01f;
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        vec3_subtract(camera->position, camera->position, vec3_multiply_f(temp, camera->up, speed));
        camera->pitch += 0.22f;
        cameraRadius += 0.01f;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void cursor_enter_callback(GLFWwindow* window, int entered)
{
    if (entered) {
        // The cursor entered the content area of the window
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        cursorEntered = true;
    } else {
        // The cursor left the content area of the window
    }
}
void instantiateVerlets(VerletObject* objects, int size)
{
    int distance = 7.0f;
    ParticleColor colors[] = {RED, GREEN, BLUE};
    int numColors = sizeof(colors)/sizeof(colors[0]);
    printf("Hello size: %d\n ", size); 
    for (int i = 0; i < size; i++) {

        VerletObject* obj = &(objects[i]);

        // ====== LOOP ======
        float x = MSIN(i) * distance;
        float z = MCOS(i) * distance;
        float xp = MSIN(i) * distance * 0.999;
        float zp = MCOS(i) * distance * 0.999;
        float y = 0;
        vec3(obj->current, x, y, z);
        vec3(obj->previous, xp, y, zp);
        vec3(obj->acceleration, 0, 0, 0);
        obj->radius = VERLET_RADIUS;
        obj->color = colors[i % numColors];
        setColorVector(obj);
        // Initialize mass from color
        setMassFromColor(obj);
        //obj->numBonds = 0;
        //obj->bonds = NULL;  // Initially no bonds 
    }
}
void spawnVerlet(VerletObject* objects, int* numActive, int maxInstances, mfloat_t* position, mfloat_t* velocity, ParticleColor color, mfloat_t radius)
{
    if (*numActive >= maxInstances) {
        return; // Cannot spawn more objects
    }

    VerletObject* obj = &objects[*numActive];
    vec3(obj->current, position[0], position[1], position[2]);
    vec3(obj->previous, position[0] - velocity[0], position[1] - velocity[1], position[2] - velocity[2]);
    vec3(obj->acceleration, 0, 0, 0);
    obj->radius = radius;
    obj->color = color;
    setColorVector(obj);
    // Initialize mass from color
    setMassFromColor(obj);
    obj->visible = true;

    (*numActive)++;
}
