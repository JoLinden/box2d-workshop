#define _USE_MATH_DEFINES
#include <iostream>
#include <stdio.h>
#include <chrono>
#include <thread>
#include <set>
#include <vector>
#include <time.h>
#include <math.h>
#include <stdlib.h>

#include "imgui/imgui.h"
#include "imgui_impl_glfw_game.h"
#include "imgui_impl_opengl3_game.h"
#include "glad/gl.h"
#include "GLFW/glfw3.h"
#include "draw_game.h"

#include "box2d/box2d.h"

// GLFW main window pointer
GLFWwindow* g_mainWindow = nullptr;

b2World* g_world;
std::set<b2Body*> bodiesToRemove;

class CollisionListener : public b2ContactListener {
public:
    void BeginContact(b2Contact* contact) override {
        while (contact != nullptr) {
            if (contact->IsTouching()) {
                b2Body* bodyA = contact->GetFixtureA()->GetBody();
                b2Body* bodyB = contact->GetFixtureB()->GetBody();
                
                if (bodyA->GetType() == b2_dynamicBody &&
                    bodyB->GetType() == b2_dynamicBody) {
                    bodiesToRemove.insert(bodyA);
                    bodiesToRemove.insert(bodyB);
                }
            }
            contact = contact->GetNext();
        }
    }

    void EndContact(b2Contact* contact) override {
        return;
    }
};

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // code for keys here https://www.glfw.org/docs/3.3/group__keys.html
    // and modifiers https://www.glfw.org/docs/3.3/group__mods.html
}

void MouseMotionCallback(GLFWwindow*, double xd, double yd)
{
    // get the position where the mouse was pressed
    b2Vec2 ps((float)xd, (float)yd);
    // now convert this position to Box2D world coordinates
    b2Vec2 pw = g_camera.ConvertScreenToWorld(ps);
}

void MouseButtonCallback(GLFWwindow* window, int32 button, int32 action, int32 mods)
{
    // code for mouse button keys https://www.glfw.org/docs/3.3/group__buttons.html
    // and modifiers https://www.glfw.org/docs/3.3/group__buttons.html
    // action is either GLFW_PRESS or GLFW_RELEASE
    double xd, yd;
    // get the position where the mouse was pressed
    glfwGetCursorPos(g_mainWindow, &xd, &yd);
    b2Vec2 ps((float)xd, (float)yd);
    // now convert this position to Box2D world coordinates
    b2Vec2 pw = g_camera.ConvertScreenToWorld(ps);
}

void create_player(float x, float y) {
    b2Body* box;
    b2PolygonShape box_shape;
    box_shape.SetAsBox(0.4f, 2.0f);
    b2FixtureDef box_fd;
    box_fd.shape = &box_shape;
    box_fd.density = 20.0f;
    box_fd.friction = 0.1f;
    b2BodyDef box_bd;
    box_bd.position.Set(x, y);
    box = g_world->CreateBody(&box_bd);
    box->CreateFixture(&box_fd);
}

void create_wall(b2Vec2 p1, b2Vec2 p2) {
    b2Body* wall;
    b2EdgeShape wall_shape;
    wall_shape.SetTwoSided(p1, p2);
    b2BodyDef wall_bd;
    wall = g_world->CreateBody(&wall_bd);
    wall->CreateFixture(&wall_shape, 0.0f);
}

void create_ball() {
    b2Body* ball;
    b2CircleShape ball_shape;
    ball_shape.m_radius = 1.0f;
    b2FixtureDef ball_fd;
    ball_fd.shape = &ball_shape;
    ball_fd.density = 20.0f;
    ball_fd.friction = 0.1f;
    ball_fd.restitution = 1.0f;
    b2BodyDef ball_bd;
    ball_bd.type = b2_dynamicBody;
    ball_bd.position.Set(0.0f, 20.0f);
    ball = g_world->CreateBody(&ball_bd);
    ball->CreateFixture(&ball_fd);


    // Calculate a random direction between -45 and 45 deg or -135 and 135 deg with velocity corresponding to below
    std::vector<int> directions = {0, 15, 30, 45, 135, 150, 175, 180, 195, 210, 225, 315, 330, 345};
    srand(time(NULL));
    int random_direction_index = rand() % directions.size();
    std::cout << directions[random_direction_index] << "\n";
    float direction = directions[random_direction_index] * M_PI/180.0f;
    float magnitude = 20.0f;
    float x = magnitude * cos(direction);
    float y = magnitude * sin(direction);
    std::cout << cos(direction) << "\n";
    std::cout << direction << ": (" << x << ", " << y << ")\n";
    ball->SetLinearVelocity(b2Vec2(x, y));
}

void setup_field() {
    create_wall(b2Vec2(-40.0f, 0.0f), b2Vec2(40.0f, 0.0f));
    create_wall(b2Vec2(-40.0f, 40.0f), b2Vec2(40.0f, 40.0f));

    create_player(35.0f, 20.0f);
    create_player(-35.0f, 20.0f);

    create_ball();
}

int main()
{

    // glfw initialization things
    if (glfwInit() == 0) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    g_mainWindow = glfwCreateWindow(g_camera.m_width, g_camera.m_height, "My game", NULL, NULL);

    if (g_mainWindow == NULL) {
        fprintf(stderr, "Failed to open GLFW g_mainWindow.\n");
        glfwTerminate();
        return -1;
    }

    // Set callbacks using GLFW
    glfwSetMouseButtonCallback(g_mainWindow, MouseButtonCallback);
    glfwSetKeyCallback(g_mainWindow, KeyCallback);
    glfwSetCursorPosCallback(g_mainWindow, MouseMotionCallback);

    glfwMakeContextCurrent(g_mainWindow);

    // Load OpenGL functions using glad
    int version = gladLoadGL(glfwGetProcAddress);

    // Setup Box2D world and with some gravity
    b2Vec2 gravity;
    gravity.Set(0.0f, 0.0f);
    g_world = new b2World(gravity);

    // Create debug draw. We will be using the debugDraw visualization to create
    // our games. Debug draw calls all the OpenGL functions for us.
    g_debugDraw.Create();
    g_world->SetDebugDraw(&g_debugDraw);
    CreateUI(g_mainWindow, 20.0f /* font size in pixels */);

    CollisionListener collision;
    g_world->SetContactListener(&collision);

    setup_field();

    // This is the color of our background in RGB components
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Control the frame rate. One draw per monitor refresh.
    std::chrono::duration<double> frameTime(0.0);
    std::chrono::duration<double> sleepAdjust(0.0);

    // Main application loop
    while (!glfwWindowShouldClose(g_mainWindow)) {
        // Use std::chrono to control frame rate. Objective here is to maintain
        // a steady 60 frames per second (no more, hopefully no less)
        std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();

        glfwGetWindowSize(g_mainWindow, &g_camera.m_width, &g_camera.m_height);

        int bufferWidth, bufferHeight;
        glfwGetFramebufferSize(g_mainWindow, &bufferWidth, &bufferHeight);
        glViewport(0, 0, bufferWidth, bufferHeight);

        // Clear previous frame (avoid creates shadows)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Setup ImGui attributes so we can draw text on the screen. Basically
        // create a window of the size of our viewport.
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(float(g_camera.m_width), float(g_camera.m_height)));
        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::Begin("Overlay", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
        ImGui::End();

        // Enable objects to be draw
        uint32 flags = 0;
        flags += b2Draw::e_shapeBit;
        g_debugDraw.SetFlags(flags);

        // When we call Step(), we run the simulation for one frame
        float timeStep = 60 > 0.0f ? 1.0f / 60 : float(0.0f);
        g_world->Step(timeStep, 8, 3);

        // Render everything on the screen
        g_world->DebugDraw();
        g_debugDraw.Flush();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(g_mainWindow);

        // Process events (mouse and keyboard) and call the functions we
        // registered before.
        glfwPollEvents();

        // Throttle to cap at 60 FPS. Which means if it's going to be past
        // 60FPS, sleeps a while instead of doing more frames.
        std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
        std::chrono::duration<double> target(1.0 / 60.0);
        std::chrono::duration<double> timeUsed = t2 - t1;
        std::chrono::duration<double> sleepTime = target - timeUsed + sleepAdjust;
        if (sleepTime > std::chrono::duration<double>(0)) {
            // Make the framerate not go over by sleeping a little.
            std::this_thread::sleep_for(sleepTime);
        }
        std::chrono::steady_clock::time_point t3 = std::chrono::steady_clock::now();
        frameTime = t3 - t1;

        // Compute the sleep adjustment using a low pass filter
        sleepAdjust = 0.9 * sleepAdjust + 0.1 * (target - frameTime);

    }

    // Terminate the program if it reaches here
    glfwTerminate();
    g_debugDraw.Destroy();
    delete g_world;

    return 0;
}
