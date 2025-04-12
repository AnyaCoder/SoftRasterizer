#include <iostream>
#include "math/matrix.h" // 假设你的 mat4 类在 mat4.h 中

// 检查两个矩阵是否近似相等
bool isApproximatelyEqual(const mat4& a, const mat4& b, float epsilon = 1e-6f) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (std::fabs(a.m[i][j] - b.m[i][j]) > epsilon) {
                return false;
            }
        }
    }
    return true;
}

// 打印矩阵
void printMatrix(const mat4& mat) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            std::cout << mat.m[i][j] << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "----------" << std::endl;
}

int test_main() {
    // 测试 1: 平移矩阵
    mat4 trans = mat4::translation(2.0f, 3.0f, 4.0f);
    mat4 transInv = trans.inverse();
    mat4 result1 = trans * transInv;
    mat4 identity = mat4::identity();

    std::cout << "Testing Translation Matrix:\n";
    printMatrix(result1);
    if (isApproximatelyEqual(result1, identity)) {
        std::cout << "Translation inverse is correct!\n";
    } else {
        std::cout << "Translation inverse failed!\n";
    }

    // 测试 2: 缩放矩阵
    mat4 scale = mat4::scale(2.0f, 0.5f, 1.5f);
    mat4 scaleInv = scale.inverse();
    mat4 result2 = scale * scaleInv;

    std::cout << "Testing Scale Matrix:\n";
    printMatrix(result2);
    if (isApproximatelyEqual(result2, identity)) {
        std::cout << "Scale inverse is correct!\n";
    } else {
        std::cout << "Scale inverse failed!\n";
    }

    // 测试 3: 旋转矩阵 (绕 Z 轴旋转 90 度)
    mat4 rotZ = mat4::rotationZ(1.5708f); // 约 90 度
    mat4 rotZInv = rotZ.inverse();
    mat4 result3 = rotZ * rotZInv;

    std::cout << "Testing RotationZ Matrix:\n";
    printMatrix(result3);
    if (isApproximatelyEqual(result3, identity)) {
        std::cout << "RotationZ inverse is correct!\n";
    } else {
        std::cout << "RotationZ inverse failed!\n";
    }

    // 测试 4: 透视投影矩阵
    mat4 persp = mat4::perspective(1.047f, 4.0f / 3.0f, 0.1f, 100.0f); // 60度 FOV, 4:3 宽高比
    mat4 perspInv = persp.inverse();
    mat4 result4 = persp * perspInv;

    std::cout << "Testing Perspective Matrix:\n";
    printMatrix(result4);
    if (isApproximatelyEqual(result4, identity)) {
        std::cout << "Perspective inverse is correct!\n";
    } else {
        std::cout << "Perspective inverse failed!\n";
    }

    return 0;
}