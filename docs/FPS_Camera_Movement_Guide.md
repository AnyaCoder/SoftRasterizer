---
title: Implementing FPS Camera Movement in a 3D Application
date: 2025-04-24
tags: [FPS, Camera, 3D, SDL, C++]
categories: [Game Development]
---

# Implementing FPS Camera Movement in a 3D Application

First-Person Shooter (FPS) camera movement is a core mechanic in many 3D applications, providing an immersive perspective where the camera mimics the viewpoint of a character. This guide details how to implement an FPS-style camera based on the provided codebase changes, which use C++, SDL, and a custom 3D rendering engine. The implementation covers mouse-based look controls, keyboard-based movement, and integration with a scene management system.

## Overview of Changes

The provided diff modifies several files to enable FPS-style camera controls. Key changes include:

- **Camera Class Overhaul** (`camera.h`, `camera.cpp`): The `Camera` class now uses yaw and pitch for orientation instead of a target-based look-at system, enabling smoother FPS-style mouse look. New methods handle keyboard and mouse input for movement and rotation.
- **SDL Application Enhancements** (`sdl_app.h`, `sdl_app.cpp`): Input handling now tracks keyboard states and mouse motion, with a toggle for mouse look mode (using the Escape key).
- **Scene Loading** (`scene.yaml`, `scene.cpp`): The scene configuration supports yaw and pitch for camera initialization, replacing the target-based setup.
- **Math Utilities** (`vector.h`, `quaternion.h`, `transform.cpp`): Optimizations like `lengthSq()` and improved numerical stability for vector and quaternion operations.
- **Miscellaneous**:
  - `CMakeLists.txt`: Switches ImGui to a static library.
  - `blinn_phong_shader.cpp`, `model.cpp`: Minor optimizations using `lengthSq()` for performance.
  - `resource_manager.cpp`, `main.cpp`: Minor cleanup and logging improvements.

This guide focuses on the FPS camera implementation, explaining the core components and how they integrate.

## Step-by-Step Implementation

### 1. Camera Class Design

The `Camera` class (`include/core/camera.h`, `src/core/camera.cpp`) is the heart of the FPS camera system. It manages position, orientation (via yaw and pitch), and projection matrices for rendering.

#### Key Features
- **Constructor**: Initializes the camera with a position, yaw, and pitch, defaulting to a forward-facing view (yaw = -90°, pitch = 0°).
  ```cpp
  Camera::Camera(const vec3f& position, float initialYaw, float initialPitch)
      : m_yaw(initialYaw), m_pitch(initialPitch) {
      m_transform.position = position;
      m_projMatrix = mat4::identity();
      updateCameraVectors();
  }
  ```
- **Orientation**: Uses yaw (Y-axis rotation) and pitch (X-axis rotation) to compute a quaternion-based rotation, avoiding gimbal lock compared to Euler angles.
- **Movement**: Supports keyboard-driven movement (WASD, Space, Ctrl) and mouse-driven look controls.
- **View Matrix**: Computed using a stable look-at construction based on the camera’s forward vector.

#### Orientation and Rotation
The camera’s orientation is defined by:
- **Yaw**: Rotation around the world’s Y-axis (up).
- **Pitch**: Rotation around the camera’s local X-axis (right).

The `updateRotationAndVectors` method computes the rotation quaternion:
```cpp
void Camera::updateRotationAndVectors() {
    quat yawQuat = quat::fromAxisAngle(m_worldUp, m_yaw * Q_DEG2RAD);
    vec3f localRight = vec3f{1.0f, 0.0f, 0.0f};
    quat pitchQuat = quat::fromAxisAngle(localRight, m_pitch * Q_DEG2RAD);
    m_transform.rotation = yawQuat * pitchQuat;
    m_transform.rotation.normalize();
}
```
- Yaw is applied first (global rotation), then pitch (local rotation), ensuring intuitive FPS controls.
- The rotation is normalized to prevent numerical drift.

The view matrix is updated in `updateViewMatrix` using the camera’s forward, right, and up vectors, derived from the rotation:
```cpp
void Camera::updateViewMatrix() {
    vec3f position = m_transform.position;
    vec3f forward = getForward();
    vec3f targetPoint = position + forward;
    vec3f actualForward = (targetPoint - position).normalized();
    vec3f actualRight = actualForward.cross(m_worldUp).normalized();
    if (actualRight.lengthSq() < 1e-6f) {
        quat yawQuatOnly = quat::fromAxisAngle(m_worldUp, m_yaw * Q_DEG2RAD);
        actualRight = yawQuatOnly * vec3f{1.0f, 0.0f, 0.0f};
    }
    vec3f actualUp = actualRight.cross(actualForward).normalized();
    mat4 rotation = mat4::identity();
    rotation.m[0][0] = actualRight.x; rotation.m[0][1] = actualRight.y; rotation.m[0][2] = actualRight.z;
    rotation.m[1][0] = actualUp.x;    rotation.m[1][1] = actualUp.y;    rotation.m[1][2] = actualUp.z;
    rotation.m[2][0] = -actualForward.x; rotation.m[2][1] = -actualForward.y; rotation.m[2][2] = -actualForward.z;
    mat4 translation = mat4::translation(-position.x, -position.y, -position.z);
    m_viewMatrix = rotation * translation;
}
```
This handles edge cases (e.g., looking straight up/down) to prevent gimbal lock or instability.

#### Mouse Look
Mouse movement adjusts yaw and pitch:
```cpp
void Camera::processMouseMovement(float xoffset, float yoffset, float sensitivity, bool constrainPitch) {
    xoffset *= sensitivity;
    yoffset *= sensitivity;
    m_yaw += xoffset;
    m_pitch += yoffset;
    m_yaw = fmod(m_yaw, 360.0f);
    if (m_yaw < 0.0f) m_yaw += 360.0f;
    if (constrainPitch) {
        m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
    }
    updateRotationAndVectors();
    updateViewMatrix();
}
```
- **Sensitivity**: Scales mouse input for smoother control.
- **Pitch Constraint**: Limits pitch to ±89° to prevent flipping at the poles.
- **Yaw Wrapping**: Keeps yaw in [0, 360°) for continuity.

#### Keyboard Movement
Keyboard input moves the camera along its forward, right, and world-up axes:
```cpp
void Camera::processKeyboardMovement(const vec3f& direction, float deltaTime, float speed) {
    float velocity = speed * deltaTime;
    vec3f moveAmount = {0.0f, 0.0f, 0.0f};
    vec3f currentForward = getForward();
    vec3f horizontalRight = -m_worldUp.cross(currentForward).normalized();
    moveAmount = moveAmount + currentForward * direction.z * velocity;
    moveAmount = moveAmount + horizontalRight * direction.x * velocity;
    moveAmount = moveAmount + m_worldUp * direction.y * velocity;
    m_transform.position = m_transform.position + moveAmount;
    updateViewMatrix();
}
```
- **Direction**: A vector where `x` is strafe (left/right), `y` is vertical (up/down), and `z` is forward/backward.
- **Delta Time**: Ensures frame-rate-independent movement.
- **Speed**: Controls movement speed (default: 5 units/second).

### 2. Input Handling in SDLApp

The `SDLApp` class (`include/core/sdl_app.h`, `src/core/sdl_app.cpp`) processes user input and updates the camera.

#### Keyboard Input
Keyboard state is tracked using an `std::unordered_set` for pressed keys:
```cpp
std::unordered_set<SDL_Scancode> keysPressed;
```
The `handleEvents` method updates this set:
```cpp
if (!io.WantCaptureKeyboard) {
    switch (event.type) {
        case SDL_KEYDOWN:
            if (event.key.repeat == 0) {
                keysPressed.insert(event.key.keysym.scancode);
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                    mouseLookActive = !mouseLookActive;
                    std::cout << "Escape Toggle: mouseLookActive = " << mouseLookActive << std::endl;
                }
            }
            break;
        case SDL_KEYUP:
            keysPressed.erase(event.key.keysym.scancode);
            break;
    }
}
```
- **ImGui Integration**: Input is ignored if ImGui (UI) wants keyboard focus.
- **Escape Key**: Toggles mouse look mode.

The `processInput` method maps keys to movement:
```cpp
void SDLApp::processInput(float dt) {
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
```
- **Normalization**: Ensures diagonal movement (e.g., W+A) doesn’t exceed the intended speed.
- **WASD Controls**: Standard FPS movement (W: forward, S: backward, A: strafe left, D: strafe right).
- **Vertical Movement**: Space (up) and Ctrl (down) allow free-fly movement, typical in debug or creative modes.

#### Mouse Input
Mouse look is enabled when `mouseLookActive` is true, using SDL’s relative mouse mode:
```cpp
if (mouseLookActive) {
    int mouseXRel, mouseYRel;
    SDL_GetRelativeMouseState(&mouseXRel, &mouseYRel);
    if (mouseXRel != 0 || mouseYRel != 0) {
        scene.getCamera().processMouseMovement(
            -static_cast<float>(mouseXRel),
            -static_cast<float>(mouseYRel),
            cameraLookSensitivity
        );
    }
}
```
- **Relative Mouse Mode**: Captures mouse movement without cursor bounds, ideal for FPS controls.
- **Sensitivity**: Adjustable via `cameraLookSensitivity` (default: 0.1).
- **Cursor Visibility**: Hidden when mouse look is active, shown otherwise.

The `handleEvents` method toggles mouse mode and cursor visibility:
```cpp
bool shouldBeRelative = mouseLookActive && !imguiCapturedMouseThisPoll;
bool currentRelativeState = SDL_GetRelativeMouseMode();
if (shouldBeRelative != currentRelativeState) {
    if (SDL_SetRelativeMouseMode(shouldBeRelative ? SDL_TRUE : SDL_FALSE) == 0) {
        SDL_ShowCursor(shouldBeRelative ? SDL_DISABLE : SDL_ENABLE);
    }
}
```
- **ImGui Compatibility**: Mouse look is disabled if ImGui captures the mouse (e.g., for UI interaction).

### 3. Scene Configuration

The scene file (`scenes/scene.yaml`) initializes the camera:
```yaml
camera:
  position: [0, 1, 3]
  yaw: 0.0
  pitch: 0.0
  width: 800
  height: 800
  fov: 45.0
  near: 0.1
  far: 100.0
```
- **Position**: Starting point (x, y, z).
- **Yaw/Pitch**: Initial orientation.
- **Perspective**: Field of view (FOV), aspect ratio, and clipping planes.

The `Scene` class (`src/core/scene.cpp`) loads these settings:
```cpp
if (cameraNode) {
    vec3f position = {0.0f, 0.0f, 5.0f};
    float yaw = -90.0f;
    float pitch = 0.0f;
    if (cameraNode["position"]) position = cameraNode["position"].as<std::vector<float>>();
    if (cameraNode["yaw"]) yaw = cameraNode["yaw"].as<float>();
    if (cameraNode["pitch"]) pitch = cameraNode["pitch"].as<float>();
    camera.setPosition(position);
    camera.setPitchYaw(pitch, yaw);
    float fov = 60.0f, aspect = 1.0f, near = 0.1f, far = 100.0f;
    if (cameraNode["fov"]) fov = cameraNode["fov"].as<float>();
    if (cameraNode["width"] && cameraNode["height"] && cameraNode["height"].as<float>() != 0) {
        aspect = cameraNode["width"].as<float>() / cameraNode["height"].as<float>();
    }
    camera.setPerspective(fov, aspect, near, far);
}
```
- **Defaults**: Provides fallback values if YAML fields are missing.
- **Flexible Aspect Ratio**: Supports width/height or direct aspect ratio.

### 4. ImGui Integration

The ImGui interface (`sdl_app.cpp`) displays camera properties and controls:
```cpp
ImGui::Begin("Inspector");
if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
    vec3f position = scene.getCamera().getPosition();
    vec3f rotation = scene.getCamera().getTransform().rotation.toEulerAnglesZYX();
    ImGui::InputFloat3("Position##Cam", &position.x, "%.2f", ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat3("Rotation##Cam", &rotation.x, "%.2f", ImGuiInputTextFlags_ReadOnly);
    ImGui::DragFloat("Move Speed", &cameraMoveSpeed, 0.1f, 0.1f, 100.0f);
    ImGui::DragFloat("Look Sensitivity", &cameraLookSensitivity, 0.01f, 0.01f, 1.0f);
    bool mouseLookStatus = mouseLookActive;
    if (ImGui::Checkbox("Mouse Look Active (Esc)", &mouseLookStatus)) {
        mouseLookActive = mouseLookStatus;
        SDL_SetRelativeMouseMode(mouseLookActive ? SDL_TRUE : SDL_FALSE);
        SDL_ShowCursor(mouseLookActive ? SDL_DISABLE : SDL_ENABLE);
    }
}
ImGui::End();
```
- **Read-Only Display**: Shows position and rotation for debugging.
- **Adjustable Parameters**: Allows tweaking movement speed and look sensitivity.
- **Mouse Look Toggle**: Mirrors the Escape key functionality.

### 5. Math Optimizations

The math library (`vector.h`, `quaternion.h`, `transform.cpp`) supports the camera with:
- **Vector3**: Added `lengthSq()` for faster length checks without square roots:
  ```cpp
  float lengthSq() const { return x * x + y * y + z * z; }
  ```
- **Quaternion**: Improved numerical stability in `toAxisAngle` and `toEulerAnglesZYX`:
  ```cpp
  if (axis.lengthSq() < 1e-6f) axis = vec3f(0.0f, 0.0f, 1.0f);
  ```
- **Transform**: Enhanced `lookAt` with robust handling of edge cases (e.g., parallel vectors).

These optimizations reduce computational overhead and improve stability for camera calculations.

## Integration with the Application

The FPS camera is integrated into the main loop in `SDLApp::run` (`sdl_app.cpp`):
```cpp
while (!quit) {
    updateFPS();
    processInput(deltaTime);
    update(deltaTime);
    renderFrame();
    updateTextureFromFramebuffer();
    renderImGui();
    render();
}
```
- **Input Processing**: `processInput` updates the camera based on keyboard and mouse input.
- **Rendering**: The camera’s view and projection matrices are passed to the renderer for scene rendering.
- **ImGui**: Provides real-time feedback and control.

## Best Practices and Tips

1. **Frame-Rate Independence**: Always scale movement by `deltaTime` to ensure consistent speed across hardware.
2. **Numerical Stability**: Use `lengthSq()` for comparisons and normalize quaternions to prevent drift.
3. **User Comfort**: Constrain pitch to avoid disorienting flips and provide adjustable sensitivity.
4. **ImGui Integration**: Ensure input is disabled when ImGui is active to prevent conflicts.
5. **Debugging**: Use ImGui to display camera state and log warnings for YAML parsing errors.

## Potential Enhancements

- **Collision Detection**: Prevent the camera from moving through objects.
- **Smoothing**: Add interpolation for smoother mouse look.
- **Configurable Keybindings**: Allow users to remap WASD controls.
- **Camera Shake**: Implement for visual effects (e.g., explosions).
- **Field of View Adjustment**: