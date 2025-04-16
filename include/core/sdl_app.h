// include/core/sdl_app.h
#pragma once
#include <SDL.h>
#include <string>
#include <functional>
#include "core/framebuffer.h"

class SDLApp {
public:
    SDLApp(int width, int height, const std::string& title);
    ~SDLApp();

    bool initialize();
    // Run the application with a render callback that returns the framebuffer
    void run(const std::function<const Framebuffer&(float)>& renderCallback);
    void updateTextureFromFramebuffer(const Framebuffer& framebuffer);
    SDL_Renderer* getRenderer() { return sdlRenderer; }

private:
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

    void handleEvents();
    void updateFPS();
};