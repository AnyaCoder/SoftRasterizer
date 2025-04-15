---
title: 使用四元数和变换类管理对象姿态
date: 2025-04-15 22:55:00
tags:
  - 图形学
  - 渲染
  - 变换
  - 四元数
  - 欧拉角
  - C++
  - 软渲染器
categories:
  - Computer Graphics
  - 技术分享
---

在之前的开发中，我们通过直接传递一个 `mat4 modelMatrix` 到渲染函数来表示模型的位置、旋转和缩放。虽然可行，但这种方式不够直观，难以单独修改某个变换分量（如只改变旋转），并且在处理旋转时可能遇到欧拉角的固有问题。为了更优雅、健壮地管理对象姿态，我们引入了 `Transform` 类，并使用**四元数 (Quaternion)** 来处理旋转。本文将详细介绍四元数的基本原理、与欧拉角的转换关系，以及如何通过 `Transform` 类来封装这些操作。

详细内容可参考这篇PDF： https://krasjet.github.io/quaternion/quaternion.pdf

## 1. 旋转的挑战：欧拉角与万向锁

我们通常习惯使用欧拉角（如绕 X、Y、Z 轴旋转的角度）来描述旋转，因为它非常直观。然而，欧拉角存在一个著名的问题——**万向锁 (Gimbal Lock)**。

当按顺序应用三个旋转时（例如，先绕 Z 轴，再绕新 X 轴，最后绕新 Y 轴），中间的旋转（绕 X 轴）可能恰好使得最后一个旋转轴（Y 轴）与第一个旋转轴（Z 轴）重合。这时，无论如何调整第一个和最后一个角度，都只能在同一个平面上旋转，丢失了一个旋转自由度，导致无法实现某些期望的旋转组合。

{% asset_img gimbal_lock.gif Gimbal Lock Illustration %}
*（示意图：万向锁现象）*

## 2. 更好的选择：四元数 (Quaternion)

四元数提供了一种更稳健、更高效的方式来表示三维空间中的旋转。

### 2.1 基本定义

一个四元数 `q` 可以表示为：
`q = w + xi + yj + zk`
其中 `w` 是实部（标量部分），`(x, y, z)` 是虚部（向量部分），`i, j, k` 是虚数单位，满足以下关系：
* `i^2 = j^2 = k^2 = ijk = -1`
* `ij = k, ji = -k`
* `jk = i, kj = -i`
* `ki = j, ik = -j`

在我们的 C++ 实现中，通常用四个浮点数表示：
```cpp
class Quaternion {
public:
    float w, x, y, z;
    // ... methods ...
};
```

### 2.2 轴角表示法
任意一个三维旋转都可以表示为绕某个单位向量轴 `a = (ax, ay, az)` 旋转角度 `θ`。这可以方便地转换为四元数：
* `w = cos(θ / 2)`
* `x = ax * sin(θ / 2)`
* `y = ay * sin(θ / 2)`
* `z = az * sin(θ / 2)`

或者写成 `q = (cos(θ/2), sin(θ/2) * a)`。

用于表示旋转的四元数必须是单位四元数，即其模长为 1。

### 2.3 关键运算与推导
* 模长 (Magnitude):
`|q| = sqrt(w^2 + x^2 + y^2 + z^2)`
对于单位四元数，|q| = 1。

* 共轭 (Conjugate):
`q* = w - xi - yj - zk = (w, -x, -y, -z)`

* 逆 (Inverse):
`q^-1 = q* / |q|^2`
对于单位四元数 (旋转四元数)，`|q|^2 = 1`，因此其逆就是其共轭：`q^-1 = q*`。

* 乘法 (Multiplication - 旋转的组合):
    四元数乘法不满足交换律 `(q1 * q2 != q2 * q1)`。它表示旋转的组合，`q_total = q2 * q1` 表示先应用 q1 旋转，再应用 q2 旋转。
    设 `q1 = (w1, x1, y1, z1)` 和 `q2 = (w2, x2, y2, z2)`。
    `q1 * q2 = (w1 + x1i + y1j + z1k) * (w2 + x2i + y2j + z2k)`
    展开并使用 i, j, k 的关系，得到：
    * `w = w1w2 - x1x2 - y1y2 - z1z2`
    * `x = w1x2 + x1w2 + y1z2 - z1y2`
    * `y = w1y2 - x1z2 + y1w2 + z1x2`
    * `z = w1z2 + x1y2 - y1x2 + z1w2`
     
    这对应了 Quaternion::operator* 的实现。

* 向量旋转:
    使用单位四元数 q 旋转向量 v，可以通过以下公式计算：
    `v' = q * v * q^-1`

    这里需要将向量 `v = (vx, vy, vz)` 表示为一个纯虚四元数 `p = (0, vx, vy, vz)`。计算过程是先计算 `p' = q * p`，再计算 `v' = p' * q^-1`。结果 `v'` 也是一个纯虚四元数，其虚部就是旋转后的向量。
    展开这个公式可以得到一个更直接的计算方法（假设 q 已归一化）：
    令 `q_vec = (x, y, z)`
    `v' = v + 2w * (q_vec × v) + 2 * (q_vec × (q_vec × v))`
    这对应了 Quaternion::operator*(vec3f) 的优化实现。

### 2.4 转换为旋转矩阵
单位四元数可以方便地转换为 3x3 或 4x4 的旋转矩阵。对应的 4x4 旋转矩阵 M 为：

$$
M = \begin{pmatrix}
1 - 2(y^2 + z^2) & 2(xy - zw) & 2(xz + yw) & 0 \
2(xy + zw) & 1 - 2(x^2 + z^2) & 2(yz - xw) & 0 \
2(xz - yw) & 2(yz + xw) & 1 - 2(x^2 + y^2) & 0 \
0 & 0 & 0 & 1
\end{pmatrix}
$$

这对应了 Quaternion::toMatrix() 和 mat4::fromQuaternion() 的实现。

## 3. 桥接便利性：欧拉角与四元数的转换
虽然四元数内部计算优势明显，但用户输入和调试时，欧拉角更直观。因此我们需要实现两者之间的转换。

### 3.1 转换约定
欧拉角的转换结果依赖于旋转顺序和是内旋 (Intrinsic) 还是外旋 (Extrinsic)。我们选择一个常见的约定：ZYX 内旋，这通常对应于：

绕物体的局部 Z 轴旋转 Roll 角。
绕新的局部 X 轴旋转 Pitch 角。
绕更新后的局部 Y 轴旋转 Yaw 角。
等效地，这也可以看作是外旋 YXZ：先绕世界 Y 轴 (Yaw)，再绕世界 X 轴 (Pitch)，最后绕世界 Z 轴 (Roll)。

### 3.2 欧拉角 -> 四元数 (ZYX 内旋 / YXZ 外旋)
设欧拉角为 (pitch, yaw, roll)，分别对应绕 X, Y, Z 轴的旋转角度。
对应的三个单轴旋转四元数分别为：
* `q_roll = (cos(roll/2), 0, 0, sin(roll/2)) (绕 Z)`
* `q_pitch = (cos(pitch/2), sin(pitch/2), 0, 0) (绕 X)`
* `q_yaw = (cos(yaw/2), 0, sin(yaw/2), 0) (绕 Y)`

最终的组合旋转（按 Roll -> Pitch -> Yaw 的顺序应用）对应的四元数为：
`q = q_yaw * q_pitch * q_roll`

展开这个乘法（注意顺序），令 `cy = cos(yaw/2)`, `sy = sin(yaw/2)`,` cp = cos(pitch/2)`, `sp = sin(pitch/2)`, `cr = cos(roll/2)`, `sr = sin(roll/2)`，可以推导出：

* `w = cr*cp*cy + sr*sp*sy`
* `x = cr*sp*cy + sr*cp*sy`  (对应 Pitch)
* `y = cr*cp*sy - sr*sp*cy`  (对应 Yaw)
* `z = sr*cp*cy - cr*sp*sy`  (对应 Roll)

这正是 Quaternion::fromEulerAnglesZYX(vec3f(pitch, yaw, roll)) 的实现依据（注意函数参数的约定）。

### 3.3 四元数 -> 欧拉角 (ZYX 内旋 / YXZ 外旋)
从单位四元数 `q = (w, x, y, z)` 推导出欧拉角 `(pitch, yaw, roll)`：

* Pitch (绕 X 轴):
    可以从旋转矩阵的 m[2][1] (或 m[1][2]) 元素或者直接从四元数推导。
    `sin(pitch) = 2 * (w*x - y*z)`
    因此 `pitch = asin(2 * (w*x - y*z))`
    需要注意 asin 的值域是 [-pi/2, pi/2]，并将结果限制在此范围内。

* Yaw (绕 Y 轴):
    `tan(yaw) = (2*(w*y + x*z)) / (1 - 2*(x^2 + y^2)) ` (如果 cos(pitch) 不为 0)
    使用 atan2 更稳健：
    `yaw = atan2(2*(w*y + x*z), 1 - 2*(x^2 + y^2))`

* Roll (绕 Z 轴):
    `tan(roll) = (2*(w*z + x*y)) / (1 - 2*(y^2 + z^2))`     (如果 cos(pitch) 不为 0)
    使用 atan2 更稳健：
    roll = atan2(2*(w*z + x*y), 1 - 2*(y^2 + z^2))

* 万向锁处理: 当 `pitch` 接近 `+/- pi/2` 时 (sin(pitch) 接近 +/- 1)，cos(pitch) 接近 0，此时发生万向锁。Yaw 和 Roll 轴发生重合，无法唯一确定。这时 `w*x - y*z` 接近 `+/- 0`.5。在这种情况下，通常约定将 Roll 设为 0，然后计算 Yaw：
`yaw = atan2(2*(w*y + x*z), 1 - 2*(x^2 + y^2)) ` (当 pitch = +pi/2)
(或者 yaw = atan2(-2*(wy - xz), ...) 根据具体情况调整)
或者使用 yaw = 2 * atan2(y, w) （当 pitch = +pi/2 且 roll = 0）

我们的 Quaternion::toEulerAnglesZYX() 实现中包含了对 asin 输入的钳制和使用 atan2，是比较标准的转换方法。

## 4. 封装变换：Transform 类
为了将位置、旋转（四元数）和缩放（向量）统一管理，我们创建了 Transform 类。

```cpp
// include/math/transform.h (部分)
class Transform {
public:
    vec3f position;      // 位置
    Quaternion rotation; // 旋转 (内部使用四元数)
    vec3f scale;         // 缩放

    // 构造函数 (包括使用欧拉角的版本)
    Transform();
    Transform(const vec3f& pos, const Quaternion& rot, const vec3f& scl);
    Transform(const vec3f& pos, const vec3f& eulerAnglesDegreesZYX, const vec3f& scl);

    // 设置/获取方法 (包括欧拉角版本)
    void setPosition(const vec3f& pos);
    void setRotation(const Quaternion& rot);
    void setScale(const vec3f& scl);
    void setRotationEulerZYX(const vec3f& eulerAnglesDegreesZYX);

    const vec3f& getPosition() const;
    const Quaternion& getRotation() const;
    const vec3f& getScale() const;
    vec3f getRotationEulerZYX() const; // 获取欧拉角表示

    // 应用变换的方法
    void translate(const vec3f& delta);
    void rotate(const Quaternion& delta); // 组合旋转
    void rotateEulerZYX(const vec3f& deltaEulerDegreesZYX); // 应用欧拉角增量旋转

    // 获取最终变换矩阵
    mat4 getTransformMatrix() const;
    mat4 getNormalMatrix() const; // 用于法线变换

    // 组合变换 (用于层级结构)
    Transform combine(const Transform& parent) const;
    // ... 其他辅助方法如 lookAt ...
};
```

* 核心方法：`getTransformMatrix()`

    这个方法负责将存储的 `position`, `rotation`, `scale` 组合成一个最终的 4x4 变换矩阵，供渲染管线使用。标准的组合顺序是先缩放 (Scale)，然后旋转 (Rotate)，最后平移 (Translate)。对应的矩阵乘法顺序是 `M = Matrix_Translate * Matrix_Rotate * Matrix_Scale`。
```cpp
// Transform::getTransformMatrix() 实现思路
mat4 scaleMat = mat4::scale(scale.x, scale.y, scale.z);
mat4 rotMat = rotation.toMatrix(); // 从四元数获取旋转矩阵
mat4 transMat = mat4::translation(position.x, position.y, position.z);

return transMat * rotMat * scaleMat; // T * R * S
```

* 法线变换矩阵：getNormalMatrix()
    变换法线时，不能直接使用模型矩阵，尤其是存在非均匀缩放时。需要使用模型矩阵左上角 3x3 部分的逆转置矩阵。Transform 类也提供了计算这个矩阵的方法。

## 5. 使用示例

```cpp
#include "math/transform.h"
#include "math/vector.h"
#include <iostream>

int main() {
    // 使用欧拉角创建 Transform (假设 ZYX: Pitch=45, Yaw=30, Roll=0)
    Transform myTransform({0, 0, -5}, {45.0f, 30.0f, 0.0f}, {1, 1, 1});

    // 平移
    myTransform.translate({1, 0, 0});

    // 旋转 (再绕世界 Y 轴旋转 15 度)
    Quaternion deltaRot = Quaternion::fromAxisAngle({0, 1, 0}, 15.0f * Q_DEG2RAD);
    myTransform.rotate(deltaRot); // 组合四元数旋转

    // 或者使用欧拉角增量旋转
    // myTransform.rotateEulerZYX({0.0f, 15.0f, 0.0f});

    // 获取最终矩阵给渲染器
    mat4 finalModelMatrix = myTransform.getTransformMatrix();
    mat4 finalNormalMatrix = myTransform.getNormalMatrix();

    // 获取当前姿态的欧拉角表示 (可能与输入不完全一致，尤其是多次旋转后)
    vec3f currentEuler = myTransform.getRotationEulerZYX();
    std::cout << "Current Euler ZYX (P,Y,R): " << currentEuler.x << ", "
              << currentEuler.y << ", " << currentEuler.z << std::endl;

    // renderer.drawModel(model, myTransform, material); // 传递 Transform 对象

    return 0;
}
```

## 6. 总结
通过引入 Transform 类并使用四元数作为内部旋转表示，我们实现了：

* 更好的封装: 将位置、旋转、缩放数据聚合管理。
* 避免万向锁: 内部旋转计算使用四元数，更加健壮。
* 用户便利性: 依然可以通过欧拉角接口来设置和获取旋转，方便用户理解和调试。
* 清晰的变换流程: getTransformMatrix() 明确了 S->R->T 的变换顺序。

这为我们构建更复杂的场景、动画和物理交互系统打下了坚实的基础。虽然引入了四元数和转换的数学，但其带来的稳定性和灵活性是值得的。