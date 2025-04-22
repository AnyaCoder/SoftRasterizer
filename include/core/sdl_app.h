// include/core/sdl_app.h
#pragma once
#include <SDL.h>
#include <string>
#include <functional>
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

    // UI Methods
    void handleEvents();
    void updateFPS();
    void renderImGui(); // Uses scene's public API

    // Rendering flow methods
    void update(float dt);
    void renderFrame();
    void updateTextureFromFramebuffer(); // Uses own framebuffer
};