#include <iostream>
#include "math/matrix.h" // 假设你的 Matrix4x4 类在 Matrix4x4.h 中

// 检查两个矩阵是否近似相等
bool isApproximatelyEqual(const Matrix4x4& a, const Matrix4x4& b, float epsilon = 1e-6f) {
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
void printMatrix(const Matrix4x4& mat) {
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
    Matrix4x4 trans = Matrix4x4::translation(2.0f, 3.0f, 4.0f);
    Matrix4x4 transInv = trans.inverse();
    Matrix4x4 result1 = trans * transInv;
    Matrix4x4 identity = Matrix4x4::identity();

    std::cout << "Testing Translation Matrix:\n";
    printMatrix(result1);
    if (isApproximatelyEqual(result1, identity)) {
        std::cout << "Translation inverse is correct!\n";
    } else {
        std::cout << "Translation inverse failed!\n";
    }

    // 测试 2: 缩放矩阵
    Matrix4x4 scale = Matrix4x4::scale(2.0f, 0.5f, 1.5f);
    Matrix4x4 scaleInv = scale.inverse();
    Matrix4x4 result2 = scale * scaleInv;

    std::cout << "Testing Scale Matrix:\n";
    printMatrix(result2);
    if (isApproximatelyEqual(result2, identity)) {
        std::cout << "Scale inverse is correct!\n";
    } else {
        std::cout << "Scale inverse failed!\n";
    }

    // 测试 3: 旋转矩阵 (绕 Z 轴旋转 90 度)
    Matrix4x4 rotZ = Matrix4x4::rotationZ(1.5708f); // 约 90 度
    Matrix4x4 rotZInv = rotZ.inverse();
    Matrix4x4 result3 = rotZ * rotZInv;

    std::cout << "Testing RotationZ Matrix:\n";
    printMatrix(result3);
    if (isApproximatelyEqual(result3, identity)) {
        std::cout << "RotationZ inverse is correct!\n";
    } else {
        std::cout << "RotationZ inverse failed!\n";
    }

    // 测试 4: 透视投影矩阵
    Matrix4x4 persp = Matrix4x4::perspective(1.047f, 4.0f / 3.0f, 0.1f, 100.0f); // 60度 FOV, 4:3 宽高比
    Matrix4x4 perspInv = persp.inverse();
    Matrix4x4 result4 = persp * perspInv;

    std::cout << "Testing Perspective Matrix:\n";
    printMatrix(result4);
    if (isApproximatelyEqual(result4, identity)) {
        std::cout << "Perspective inverse is correct!\n";
    } else {
        std::cout << "Perspective inverse failed!\n";
    }

    return 0;
}