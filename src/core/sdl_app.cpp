// src/core/sdl_app.cpp
#include "core/sdl_app.h"
#include "core/scene.h"
#include <iostream>
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

// Constructor initializes owned components
SDLApp::SDLApp(int w, int h, const std::string& t)
    : width(w), height(h), title(t), threadPool(std::max(1u, std::thread::hardware_concurrency() - 1)),
      window(nullptr), sdlRenderer(nullptr), framebufferTexture(nullptr), 
      quit(false), deltaTime(0.0f), frameCount(0), fps(0.0f),
      lastFrameTime(0), fpsUpdateTimer(0),
      resourceManager(), scene(w, h, resourceManager), 
      framebuffer(w, h, threadPool),
      renderer(framebuffer, threadPool) 
{
    std::cout << "Initializing SDLApp with " << threadPool.getNumThreads() << " threads." << std::endl;
}


SDLApp::~SDLApp() {
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    if (ImGui::GetCurrentContext()) {
        ImGui::DestroyContext();
    }
    if (framebufferTexture) SDL_DestroyTexture(framebufferTexture);
    if (sdlRenderer) SDL_DestroyRenderer(sdlRenderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
    std::cout << "SDLApp destroyed." << std::endl;
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
   
    // 初始化ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // 启用Docking
    ImGui::StyleColorsDark();
    // 设置编辑器风格主题
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 0.9f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.2f, 0.2f, 0.2f, 0.6f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.3f, 0.3f, 0.3f, 0.8f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.4f, 0.4f, 0.4f, 0.9f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    
    if (!ImGui_ImplSDL2_InitForSDLRenderer(window, sdlRenderer)) {
        std::cerr << "ImGui_ImplSDL2_InitForSDLRenderer failed" << std::endl;
        return false;
    }
    if (!ImGui_ImplSDLRenderer2_Init(sdlRenderer)) {
        std::cerr << "ImGui_ImplSDLRenderer2_Init failed" << std::endl;
        return false;
    }

    std::cout << "Loading scene..." << std::endl;
    if (scene.loadFromYAML("scenes/scene.yaml")) {
        std::cout << "Loaded scene!" << std::endl;
    }

    lastFrameTime = SDL_GetTicks();
    fpsUpdateTimer = lastFrameTime;
    return true;
}

void SDLApp::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event); // Pass events to ImGui
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

void SDLApp::update(float dt) {
    scene.update(dt);
}

// New function to encapsulate rendering logic
void SDLApp::renderFrame() {
    renderer.clear(vec3f(0.5f, 0.5f, 0.5f));

    renderer.setCameraParams(scene.getCamera().getViewMatrix(),
                             scene.getCamera().getProjectionMatrix(),
                             scene.getCamera().getPosition());
    renderer.setLights(scene.getLights());

    scene.render(renderer);

    framebuffer.flipVertical();
}


void SDLApp::renderImGui() {
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Load Scene...", "Ctrl+O")) { /* TODO: Implement scene loading */ }
            if (ImGui::MenuItem("Save Scene...", "Ctrl+S")) { /* TODO: Implement scene saving */ }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) { quit = true; }
            ImGui::EndMenu();
        }
         if (ImGui::BeginMenu("View")) {
             // Add toggles for different ImGui panels here
             ImGui::MenuItem("Show Demo Window"); // Example toggle
             ImGui::EndMenu();  
         }
        ImGui::EndMainMenuBar();
    }


    ImGui::Begin("Inspector");
    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        vec3f position = scene.getCamera().getPosition();
        vec3f target = scene.getCamera().getTarget();
        vec3f rotation = scene.getCamera().getTransform().rotation.toEulerAnglesZYX(); // Get Euler angles

        if (ImGui::DragFloat3("Position##Cam", &position.x, 0.1f)) {
            scene.getCamera().setPosition(position);
        }
        if (ImGui::DragFloat3("Target##Cam", &target.x, 0.1f)) {
            scene.getCamera().setTarget(target); // Make sure this updates the view matrix
        }
        if (ImGui::DragFloat3("Rotation (Euler)##Cam", &rotation.x, 1.0f)) { // Euler control
            scene.getCamera().setRotation(rotation); // Camera needs to handle this correctly
        }
    }
    ImGui::End(); // End Inspector


    ImGui::Begin("Status");
    ImGui::Text("FPS: %.1f", fps);
    ImGui::Text("Frame Time: %.3f ms", deltaTime * 1000.0f);
    ImGui::Text("Resolution: %d x %d", width, height);
    ImGui::Text("Threads: %d", threadPool.getNumThreads());
    ImGui::End(); // End Status

    ImGui::Render();
    SDL_SetRenderDrawBlendMode(sdlRenderer, SDL_BLENDMODE_BLEND);
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), sdlRenderer);
}

void SDLApp::updateTextureFromFramebuffer() {
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
    
#ifdef MultiThreading
    uint32_t numThreads = threadPool.getNumThreads();
    numThreads = std::max(1u, numThreads);
    int rowsPerThread = (height + numThreads - 1) / numThreads; // Ceiling division

    for (int startY = 0; startY < height; startY += rowsPerThread) {
        int endY = std::min(startY + rowsPerThread, height);
        threadPool.enqueue([this, dstPixels, pitch, startY, endY]() {
            auto& pixels = framebuffer.getPixels();
            for (int y = startY; y < endY; ++y) {
                for (int x = 0; x < width; ++x) {
                    int framebufferY = y;
                    const vec3f& color = pixels[framebufferY * width + x];
                    Uint8* dstPixel = dstPixels + y * pitch + x * 3;
                    dstPixel[0] = static_cast<Uint8>(std::round(color.x * 255.0f));
                    dstPixel[1] = static_cast<Uint8>(std::round(color.y * 255.0f));
                    dstPixel[2] = static_cast<Uint8>(std::round(color.z * 255.0f));
                }
            }
        });
    }
    threadPool.waitForCompletion();
#else   
    auto& pixels = framebuffer.getPixels();
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int framebufferY = y;
            const vec3f& color = pixels[framebufferY * width + x];
            Uint8* dstPixel = dstPixels + y * pitch + x * 3;
            dstPixel[0] = static_cast<Uint8>(std::round(color.x * 255.0f));
            dstPixel[1] = static_cast<Uint8>(std::round(color.y * 255.0f));
            dstPixel[2] = static_cast<Uint8>(std::round(color.z * 255.0f));
        }
    }
#endif
    SDL_UnlockTexture(framebufferTexture);
}   

// Main application loop
void SDLApp::run() {

    while (!quit) {

        handleEvents();

        updateFPS();

        update(deltaTime);

        renderFrame();

        updateTextureFromFramebuffer();

        SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255); // Optional clear before copy
        SDL_RenderClear(sdlRenderer);
        SDL_RenderCopy(sdlRenderer, framebufferTexture, nullptr, nullptr); // Blit texture to screen
        renderImGui();

        SDL_RenderPresent(sdlRenderer);
    }
    std::cout << "Exiting main loop." << std::endl;
}