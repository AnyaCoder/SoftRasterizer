// src/core/sdl_app.cpp
#include "core/sdl_app.h"
#include <iostream>

SDLApp::SDLApp(int width, int height, const std::string& title)
    : width(width), height(height), title(title), window(nullptr), sdlRenderer(nullptr),
      framebufferTexture(nullptr), quit(false), deltaTime(0.0f), frameCount(0), fps(0.0f),
      lastFrameTime(0), fpsUpdateTimer(0) {}

SDLApp::~SDLApp() {
    if (framebufferTexture) SDL_DestroyTexture(framebufferTexture);
    if (sdlRenderer) SDL_DestroyRenderer(sdlRenderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

bool SDLApp::initialize() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow(
        title.c_str(), 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height, SDL_WINDOW_SHOWN
    );
    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        return false;
    }

    sdlRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!sdlRenderer) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        return false;
    }

    framebufferTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_RGB24,
                                          SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!framebufferTexture) {
        std::cerr << "SDL_CreateTexture Error: " << SDL_GetError() << std::endl;
        return false;
    }

    lastFrameTime = SDL_GetTicks();
    fpsUpdateTimer = lastFrameTime;
    return true;
}

void SDLApp::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            quit = true;
        }
        // Add other event handling here if needed
    }
}

void SDLApp::updateFPS() {
    Uint32 currentFrameTime = SDL_GetTicks();
    deltaTime = (currentFrameTime - lastFrameTime) / 1000.0f;
    lastFrameTime = currentFrameTime;

    frameCount++;
    if (currentFrameTime - fpsUpdateTimer >= 1000) {
        fps = static_cast<float>(frameCount) / ((currentFrameTime - fpsUpdateTimer) / 1000.0f);
        fpsUpdateTimer = currentFrameTime;
        frameCount = 0;

        std::string updatedTitle = title + " - FPS: " + std::to_string(static_cast<int>(std::round(fps)));
        SDL_SetWindowTitle(window, updatedTitle.c_str());
    }
}

void SDLApp::updateTextureFromFramebuffer(const Framebuffer& framebuffer) {
    void* texturePixels;
    int pitch;
    if (SDL_LockTexture(framebufferTexture, NULL, &texturePixels, &pitch) != 0) {
        std::cerr << "SDL_LockTexture failed: " << SDL_GetError() << std::endl;
        return;
    }

    if (pitch < width * 3) {
        std::cerr << "Texture pitch too small." << std::endl;
        SDL_UnlockTexture(framebufferTexture);
        return;
    }

    Uint8* dstPixels = static_cast<Uint8*>(texturePixels);
    auto& pixels = framebuffer.getPixels();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int framebufferY = y;
            const vec3f& color = pixels[framebufferY * width + x];
            Uint8* dstPixel = dstPixels + y * pitch + x * 3;
            dstPixel[0] = static_cast<Uint8>(std::max(0.0f, std::min(255.0f, std::round(color.x * 255.0f))));
            dstPixel[1] = static_cast<Uint8>(std::max(0.0f, std::min(255.0f, std::round(color.y * 255.0f))));
            dstPixel[2] = static_cast<Uint8>(std::max(0.0f, std::min(255.0f, std::round(color.z * 255.0f))));
        }
    }

    SDL_UnlockTexture(framebufferTexture);
}

void SDLApp::run(const std::function<const Framebuffer&(float)>& renderCallback) {
    while (!quit) {
        handleEvents();
        updateFPS();

        // Call the render callback to get the framebuffer
        const Framebuffer& framebuffer = renderCallback(deltaTime);

        // Update SDL texture and render
        updateTextureFromFramebuffer(framebuffer);
        SDL_RenderClear(sdlRenderer);
        SDL_RenderCopy(sdlRenderer, framebufferTexture, nullptr, nullptr);
        SDL_RenderPresent(sdlRenderer);
    }
}