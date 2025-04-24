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
    if (mouseLookActive) {
        SDL_SetRelativeMouseMode(SDL_FALSE);
    }

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
    
    if (SDL_SetRelativeMouseMode(SDL_TRUE) != 0) {
        std::cerr << "Error enabling relative mouse mode: " << SDL_GetError() << std::endl;
    }

    // 初始化ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
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
    bool imguiCapturedMouseThisPoll = false;
    ImGuiIO& io = ImGui::GetIO();

    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event); 

        // Track if ImGui wanted the mouse at any point during polling
        if (io.WantCaptureMouse) {
            imguiCapturedMouseThisPoll = true;
        }

        // --- Keyboard Handling ---
        // Only process game key events if ImGui doesn't want keyboard focus NOW
        if (!io.WantCaptureKeyboard) {
             switch (event.type) {
                case SDL_KEYDOWN:
                    // std::cout << "Key Down (Game): " << SDL_GetKeyName(event.key.keysym.sym) << std::endl;
                    if (event.key.repeat == 0) {
                        keysPressed.insert(event.key.keysym.scancode);

                        // Allow Escape toggle regardless of ImGui focus? (User choice)
                        if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                            mouseLookActive = !mouseLookActive;
                            // State will be fully applied later
                            std::cout << "Escape Toggle: mouseLookActive = " << mouseLookActive << std::endl;
                        }
                    }
                    break;
                case SDL_KEYUP:
                     // std::cout << "Key Up (Game): " << SDL_GetKeyName(event.key.keysym.sym) << std::endl;
                    keysPressed.erase(event.key.keysym.scancode);
                    break;
             }
        } else {
            // If ImGui wants keyboard, clear our key state for this frame to prevent movement
            // Doing it here ensures keys pressed *before* focus change are cleared
            // keysPressed.clear(); // Maybe clear *after* the loop? See below.
        }

        // --- Quit Event ---
        if (event.type == SDL_QUIT) {
            quit = true;
        }
    } // End while SDL_PollEvent

    if (io.WantCaptureKeyboard) {
        // std::cout << "ImGui wants keyboard focus, clearing keysPressed." << std::endl;
        keysPressed.clear();
    }


    // Apply mouse mode state based on final status for the frame
    bool shouldBeRelative = mouseLookActive && !imguiCapturedMouseThisPoll; // Only relative if active AND ImGui didn't grab mouse during polling
    bool currentRelativeState = SDL_GetRelativeMouseMode();

    if (shouldBeRelative != currentRelativeState) {
        if (SDL_SetRelativeMouseMode(shouldBeRelative ? SDL_TRUE : SDL_FALSE) == 0) {
            SDL_ShowCursor(shouldBeRelative ? SDL_DISABLE : SDL_ENABLE);
        } else {
            std::cerr << "Error setting relative mouse mode!" << std::endl;
        }
    } else if (shouldBeRelative && SDL_ShowCursor(SDL_QUERY) == SDL_ENABLE) {
        SDL_ShowCursor(SDL_DISABLE);
    } else if (!shouldBeRelative && SDL_ShowCursor(SDL_QUERY) == SDL_DISABLE) {
        SDL_ShowCursor(SDL_ENABLE);
    }

} // End handleEvents

// --- New Method ---
void SDLApp::processInput(float dt) {
    // --- Mouse Look ---
    if (mouseLookActive) {
        int mouseXRel, mouseYRel;
        SDL_GetRelativeMouseState(&mouseXRel, &mouseYRel); // Get cumulative motion since last call

        if (mouseXRel != 0 || mouseYRel != 0) {
            scene.getCamera().processMouseMovement(
                -static_cast<float>(mouseXRel),
                -static_cast<float>(mouseYRel), // Invert Y-axis for typical FPS look
                cameraLookSensitivity // No dt scaling for mouse look usually
            );
        }
    }

    // --- Keyboard Movement ---
    vec3f moveDir = {0.0f, 0.0f, 0.0f};
    if (keysPressed.count(SDL_SCANCODE_W)) moveDir.z += 1.0f; // Forward
    if (keysPressed.count(SDL_SCANCODE_S)) moveDir.z -= 1.0f; // Backward
    if (keysPressed.count(SDL_SCANCODE_A)) moveDir.x -= 1.0f; // Left
    if (keysPressed.count(SDL_SCANCODE_D)) moveDir.x += 1.0f; // Right
    if (keysPressed.count(SDL_SCANCODE_SPACE)) moveDir.y += 1.0f; // Up
    if (keysPressed.count(SDL_SCANCODE_LCTRL) || keysPressed.count(SDL_SCANCODE_RCTRL)) moveDir.y -= 1.0f; // Down
    
    if (moveDir.lengthSq() > 1.0f) {
        moveDir.normalize();
    }
   
    if (moveDir.x != 0.0f || moveDir.y != 0.0f || moveDir.z != 0.0f) {
        scene.getCamera().processKeyboardMovement(moveDir, dt, cameraMoveSpeed);
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
    renderer.clear(vec3f(0.2f, 0.2f, 0.2f));

    renderer.setCameraParams(scene.getCamera().getViewMatrix(),
                             scene.getCamera().getProjectionMatrix(),
                             scene.getCamera().getPosition());
    renderer.setLights(scene.getLights());

    scene.render(renderer);

    // framebuffer.flipVertical();
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
        vec3f rotation = scene.getCamera().getTransform().rotation.toEulerAnglesZYX();
        ImGui::InputFloat3("Position##Cam", &position.x, "%.2f", ImGuiInputTextFlags_ReadOnly);
        ImGui::InputFloat3("Rotation##Cam", &rotation.x, "%.2f", ImGuiInputTextFlags_ReadOnly);
        ImGui::DragFloat("Move Speed", &cameraMoveSpeed, 0.1f, 0.1f, 100.0f);
        ImGui::DragFloat("Look Sensitivity", &cameraLookSensitivity, 0.01f, 0.01f, 1.0f);

        bool mouseLookStatus = mouseLookActive; // Copy status for checkbox
        if (ImGui::Checkbox("Mouse Look Active (Esc)", &mouseLookStatus)) {
            // Manually toggle if checkbox is clicked (in addition to Esc key)
            mouseLookActive = mouseLookStatus;
            SDL_SetRelativeMouseMode(mouseLookActive ? SDL_TRUE : SDL_FALSE);
            SDL_ShowCursor(mouseLookActive ? SDL_DISABLE : SDL_ENABLE);
        }
    }
    ImGui::End(); // End Inspector


    ImGui::Begin("Status");
    ImGui::Text("FPS: %.1f", fps);
    ImGui::Text("Frame Time: %.3f ms", deltaTime * 1000.0f);
    ImGui::Text("Resolution: %d x %d", width, height);
    ImGui::Text("Threads: %d", threadPool.getNumThreads());
    ImGui::Text(mouseLookActive ? "Mouse Look: ON" : "Mouse Look: OFF (Press Esc)");
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
                    int framebufferY = height - 1 - y;
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

        processInput(deltaTime);

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