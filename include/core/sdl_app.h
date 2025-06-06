// include/core/sdl_app.h
#pragma once
#include <SDL.h>
#include <SDL_scancode.h> // For SDL_SCANCODE_* constants
#include <string>
#include <functional>
#include <unordered_set> // For key states
#include "core/framebuffer.h"
#include "core/scene.h" 

class SDLApp {
public:
    SDLApp(int width, int height, const std::string& title);
    ~SDLApp();

    bool initialize();
    // Run the application with a render callback that returns the framebuffer
    void run();

private:
    ThreadPool threadPool;
    ResourceManager resourceManager;
    Framebuffer framebuffer; 
    Scene scene; 
    Renderer renderer;

    int width;
    int height;
    std::string title;
    SDL_Window* window;
    SDL_Renderer* sdlRenderer;
    SDL_Texture* framebufferTexture;
    bool quit;
    float deltaTime;
    int frameCount;
    float fps;
    Uint32 lastFrameTime;
    Uint32 fpsUpdateTimer;

    std::unordered_set<SDL_Scancode> keysPressed;
    bool mouseLookActive = true; // Start with mouse look enabled
    float cameraMoveSpeed = 5.0f;    // Units per second
    float cameraLookSensitivity = 0.1f; // Mouse sensitivity factor


    // UI Methods
    void handleEvents();
    void processInput(float dt); 
    void updateFPS();
    void renderImGui(); // Uses scene's public API

    // Rendering flow methods
    void update(float dt);
    void renderFrame();
    void updateTextureFromFramebuffer(); // Uses own framebuffer
};