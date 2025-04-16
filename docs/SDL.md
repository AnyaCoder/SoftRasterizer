---
title: 使用 SDL 窗口化实时渲染：设计 Scene 和 SDLApp 组件
date: 2025-04-16
tags:
  - SDL
  - 实时渲染
  - 软件光栅化
  - 场景管理
categories:
  - 图形学
  - 游戏开发
---

# 使用 SDL 窗口化实时渲染：设计 Scene 和 SDLApp 组件

在开发实时渲染应用时，SDL（Simple DirectMedia Layer）是一个轻量且跨平台的库，广泛用于创建窗口、处理输入和显示渲染结果。本文分享了一个基于 SDL 的软件光栅化渲染器的设计与实现，重点介绍如何通过模块化的组件（如 `Scene` 和 `SDLApp`）实现窗口化实时渲染。我们将从架构设计、组件实现、场景加载到具体代码细节逐步展开，适合对图形学、游戏开发或系统设计感兴趣的开发者参考。

## 背景与目标

目标是构建一个软件光栅化渲染器，支持加载 3D 模型（OBJ 格式）、应用 Blinn-Phong 着色、处理光照和纹理，并通过 SDL 窗口实时显示渲染结果。核心需求包括：
- **模块化设计**：将渲染逻辑与窗口/输入处理分离。
- **灵活的场景管理**：支持通过配置文件（如 YAML）动态加载场景。
- **实时交互**：实现流畅的渲染循环，支持动画和用户输入。
- **跨平台兼容**：利用 SDL 确保代码在 Windows、Linux 等平台上运行。

最终实现了一个渲染器，能够加载非洲头模型（`african_head.obj`）及其纹理，应用旋转动画，并通过 SDL 窗口显示，帧率信息实时更新在窗口标题栏。

## 架构设计

为了实现上述目标，我们设计了以下核心组件：

1. **SDLApp**：负责 SDL 窗口管理、事件处理和渲染循环。
2. **Scene**：管理渲染相关的数据（如模型、材质、光照）和逻辑。
3. **Renderer**：执行光栅化渲染，将场景绘制到帧缓冲区。
4. **Framebuffer**：存储渲染结果的像素数据，供 SDL 显示。
5. **Camera、Light、Model、Material**：场景的子组件，定义视角、光照、几何和材质。

架构图如下：

```plain
+------------------+
|      SDLApp      |
| - Window         |
| - Renderer       |
| - Texture        |
| - Event Loop     |
| - FPS Counter    |
+------------------+
         |
         v
+------------------+
|      Scene       |
| - Framebuffer    |
| - Renderer       |
| - Camera         |
| - Lights         |
| - Objects        |
|   - Model        |
|   - Material     |
|   - Transform    |
+------------------+
```

### 设计原则

- **关注点分离**：`SDLApp` 只处理窗口和输入，`Scene` 专注于渲染逻辑。
- **松耦合**：通过回调机制连接 `SDLApp` 和 `Scene`，避免直接依赖。
- **可扩展性**：使用 YAML 配置文件加载场景，支持动态修改。
- **简洁性**：保持接口清晰，代码易于维护和扩展。

## 组件设计与实现

### 1. SDLApp：窗口与渲染循环

`SDLApp` 是应用的入口，负责初始化 SDL、创建窗口、管理渲染循环和处理输入事件。其核心职责包括：
- 初始化 SDL 窗口和渲染器。
- 创建流式纹理（`SDL_Texture`）用于显示帧缓冲区。
- 运行主循环，处理事件、更新 FPS 和显示渲染结果。
- 通过回调与 `Scene` 交互，获取渲染后的帧缓冲区。

#### 关键代码

`SDLApp` 的头文件定义如下：

```cpp
class SDLApp {
public:
    SDLApp(int width, int height, const std::string& title);
    ~SDLApp();
    bool initialize();
    void run(const std::function<const Framebuffer&(float)>& renderCallback);
    void updateTextureFromFramebuffer(const Framebuffer& framebuffer);
    SDL_Renderer* getRenderer();

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
```

run 方法实现了主循环，调用渲染回调获取帧缓冲区并更新显示：

```cpp
void SDLApp::run(const std::function<Framebuffer&(float)>& renderCallback) {
    while (!quit) {
        handleEvents();
        updateFPS();
        Framebuffer& framebuffer = renderCallback(deltaTime);
        updateTextureFromFramebuffer(framebuffer);
        SDL_RenderClear(sdlRenderer);
        SDL_RenderCopy(sdlRenderer, nullptr, nullptr, nullptr);
        SDL_RenderPresent(sdlRenderer);
    }
}
```
updateTextureFromFramebuffer 将帧缓冲区的像素数据复制到 SDL 纹理：

```cpp
void SDLApp::updateTextureFromFramebuffer(Framebuffer& framebuffer) {
    void* texturePixels;
    int pitch;
    if (SDL_LockTexture(framebufferTexture, NULL, &texturePixels, &pitch) != 0) {
        std::cerr << "SDL_LockTexture failed: " << SDL_GetError() << std::endl;
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
```

设计亮点
1. 回调机制：通过 std::function<Framebuffer&(float)> 解耦 SDLApp 和渲染逻辑，允许任意组件提供帧缓冲区。
2. 事件封装：handleEvents 和 updateFPS 是私有方法，仅在 run 中调用，确保外部无法误用。
3. FPS 显示：每秒更新窗口标题，显示实时帧率，方便性能监控。

### 2. Scene：场景管理与渲染
Scene 负责管理渲染相关的数据和逻辑，包括帧缓冲区、渲染器、相机、光源和场景对象。它通过 YAML 配置文件加载场景，支持动态模型、材质和动画。

#### 数据结构
Scene 使用 SceneObject 结构体表示场景中的对象：

```cpp
struct SceneObject {
    Model model;
    Material material;
    Transform transform;
    struct Animation {
        enum class Type { None, RotateY } type = Type::None;
        float speed = 0.0f;
    } animation;
};
```

Scene 类定义如下：

```cpp
class Scene {
public:
    Scene(int width, int height);
    bool loadFromYAML(const std::string& filename);
    void update(float deltaTime);
    void render();
    Framebuffer& getFramebuffer();

private:
    Framebuffer framebuffer;
    Renderer renderer;
    Camera camera;
    std::vector<Light> lights;
    std::vector<SceneObject> objects;
    void initializeDefaultScene();
};
```

#### YAML 场景加载
场景通过 YAML 文件定义，包含相机、光源和对象。例如：
```yaml
camera:
  position: [0, 1, 3]
  target: [0, 0, 0]
  up: [0, 1, 0]
  fov: 45.0
  near: 0.1
  far: 100.0
lights:
  - type: directional
    direction: [0.707, 0.0, -0.707]
    color: [1.0, 1.0, 1.0]
    intensity: 1.0
objects:
  - name: head
    model: resources/obj/african_head.obj
    material:
      shader: blinn_phong
      diffuse_texture: resources/diffuse/african_head_diffuse.tga
      normal_texture: resources/normal_tangent/african_head_nm_tangent.tga
      specular_texture: resources/spec/african_head_spec.tga
    transform:
      position: [0.0, 0.0, 0.0]
      rotation: [0.0, 0.0, 0.0]
      scale: [1.0, 1.0, 1.0]
      animation:
        type: rotate_y
        speed: 30.0

```

loadFromYAML 方法解析 YAML 文件，初始化相机、光源和对象：

```cpp
bool Scene::loadFromYAML(const std::string& filename) {
    try {
        YAML::Node config = YAML::LoadFile(filename);
        // Load camera
        auto cameraNode = config["camera"];
        if (cameraNode) {
            vec3f position = cameraNode["position"].as<std::vector<float>>();
            vec3f target = cameraNode["target"].as<std::vector<float>>();
            vec3f up = cameraNode["up"].as<std::vector<float>>();
            camera = Camera(position, target, up);
            camera.setPerspective(
                cameraNode["fov"].as<float>(),
                static_cast<float>(framebuffer.getWidth()) / framebuffer.getHeight(),
                cameraNode["near"].as<float>(),
                cameraNode["far"].as<float>()
            );
            renderer.setCamera(camera);
        }
        // Load lights
        lights.clear();
        auto lightsNode = config["lights"];
        if (lightsNode) {
            for (const auto& lightNode : lightsNode) {
                Light light;
                std::string type = lightNode["type"].as<std::string>();
                if (type == "directional") light.type = LightType::DIRECTIONAL;
                else if (type == "point") light.type = LightType::POINT;
                else continue;
                light.color = lightNode["color"].as<std::vector<float>>();
                light.intensity = lightNode["intensity"].as<float>();
                if (light.type == LightType::DIRECTIONAL) {
                    light.direction = vec3f(lightNode["direction"].as<std::vector<float>>()).normalized();
                } else {
                    light.position = lightNode["position"].as<std::vector<float>>();
                }
                lights.push_back(light);
            }
            renderer.setLights(lights);
        }
        // Load objects
        objects.clear();
        auto objectsNode = config["objects"];
        if (objectsNode) {
            for (const auto& objNode : objectsNode) {
                SceneObject obj;
                std::string modelPath = objNode["model"].as<std::string>();
                if (!obj.model.loadFromObj(modelPath)) {
                    std::cerr << "Failed to load model: " << modelPath << std::endl;
                    continue;
                }
                auto matNode = objNode["material"];
                obj.material = Material(std::make_shared<BlinnPhongShader>());
                if (matNode["diffuse_texture"]) {
                    obj.material.loadDiffuseTexture(matNode["diffuse_texture"].as<std::string>());
                }
                if (matNode["normal_texture"]) {
                    obj.material.loadNormalTexture(matNode["normal_texture"].as<std::string>());
                }
                if (matNode["specular_texture"]) {
                    obj.material.loadSpecularTexture(matNode["specular_texture"].as<std::string>());
                }
                auto transformNode = objNode["transform"];
                if (transformNode) {
                    if (transformNode["position"]) {
                        obj.transform.setPosition(transformNode["position"].as<std::vector<float>>());
                    }
                    if (transformNode["rotation"]) {
                        obj.transform.setRotationEulerZYX(transformNode["rotation"].as<std::vector<float>>());
                    }
                    if (transformNode["scale"]) {
                        obj.transform.setScale(transformNode["scale"].as<std::vector<float>>());
                    }
                }
                auto animNode = transformNode["animation"];
                if (animNode && animNode["type"]) {
                    std::string animType = animNode["type"].as<std::string>();
                    if (animType == "rotate_y") {
                        obj.animation.type = SceneObject::Animation::Type::RotateY;
                        obj.animation.speed = animNode["speed"].as<float>();
                    }
                }
                objects.push_back(obj);
            }
        }
        return true;
    } catch (const YAML::Exception& e) {
        std::cerr << "Error parsing YAML file: " << e.what() << std::endl;
        initializeDefaultScene();
        return false;
    }
}
```

#### 设计亮点
* 泛化场景：通过 std::vector<SceneObject> 管理任意数量的模型，取代硬编码的特定模型。

* 动态加载：YAML 文件定义场景，易于修改和扩展，无需更改代码。

* 动画支持：通过 Animation 结构体实现简单的旋转动画，可扩展到更多类型。

### 3. 其他组件
* Renderer：执行光栅化管线，处理顶点变换、裁剪、光栅化和片段着色。支持 Blinn-Phong 着色模型。

* Framebuffer：存储渲染结果的像素和深度数据，支持颜色清除和垂直翻转（适配 SDL 坐标系）。

* Camera：提供视角变换矩阵，支持透视投影。

* Light：支持方向光和点光，传递给着色器。

* Model 和 Material：加载 OBJ 模型和 TGA 纹理，支持法线贴图和镜面贴图。

这些组件与 Scene 紧密协作，共同完成渲染任务。

## 集成与运行
main.cpp 负责初始化和启动应用：
```cpp
int main(int argc, char* argv[]) {
    const int width = 800;
    const int height = 800;
    SDLApp app(width, height, "Software Rasterizer");
    if (!app.initialize()) {
        std::cerr << "Failed to initialize SDLApp" << std::endl;
        return 1;
    }
    Scene scene(width, height);
    if (!scene.loadFromYAML("scenes/scene.yaml")) {
        std::cerr << "Failed to load scene, using default" << std::endl;
    }
    app.run([&scene](float deltaTime) -> Framebuffer& {
        scene.update(deltaTime);
        scene.render();
        return scene.getFramebuffer();
    });
    std::cout << "Exiting..." << std::endl;
    return 0;
}
```

## 构建与依赖
使用 CMake 管理构建，依赖 SDL2 和 yaml-cpp：
```cmake
cmake_minimum_required(VERSION 3.10)
project(SoftRasterizer)
set(CMAKE_CXX_STANDARD 17)
find_package(SDL2 REQUIRED)
add_subdirectory(thirdparty/yaml-cpp)
file(GLOB SOURCES "src/*.cpp" "src/core/*.cpp" "src/io/*.cpp" "src/math/*.cpp")
add_executable(SoftRasterizer ${SOURCES})
target_link_libraries(SoftRasterizer PRIVATE SDL2::SDL2 SDL2::SDL2main yaml-cpp)
target_include_directories(SoftRasterizer PRIVATE include ${SDL2_INCLUDE_DIRS})
add_custom_command(TARGET SoftRasterizer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/resources ${CMAKE_BINARY_DIR}/bin/resources
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/scenes/scene.yaml ${CMAKE_BINARY_DIR}/bin/scenes/scene.yaml
)
```

## 实现效果
运行后，程序加载 scene.yaml，渲染非洲头模型及其眼睛，应用 Blinn-Phong 着色和纹理。模型以每秒 30 度的速度绕 Y 轴旋转，窗口标题显示实时 FPS。效果如下：
* 渲染质量：支持法线贴图、镜面高光，视觉效果逼真。
* 性能：软件光栅化在 800x800 分辨率下流畅运行，FPS ~= 30。


## 总结与展望
通过模块化的 SDLApp 和 Scene 设计，我们实现了一个灵活的实时渲染器。SDLApp 封装了窗口和输入逻辑，Scene 通过 YAML 提供动态场景管理，回调机制确保了两者的松耦合。以下是未来可改进的方向：
* 事件处理：扩展 SDLApp 支持键盘和鼠标输入，实现相机控制。
* 多场景支持：允许运行时切换 YAML 文件，加载不同场景。
* 渲染优化：添加 SIMD 指令（如 AVX2）加速光栅化。
* 更复杂动画：支持关键帧动画或骨骼动画。

