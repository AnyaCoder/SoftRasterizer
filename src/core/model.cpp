#include "core/model.h"
#include "core/framebuffer.h"
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>

bool Model::loadFromObj(const std::string& filename) {
    std::ifstream in(filename);
    if (!in.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            Vector3<float> v;
            iss >> v.x >> v.y >> v.z;
            vertices.push_back(v);
        }
        else if (type == "vt") {
            Vector3<float> vt;
            iss >> vt.x >> vt.y;
            if (iss >> vt.z) {} // Optional w component
            texCoords.push_back(vt);
        }
        else if (type == "vn") {
            Vector3<float> vn;
            iss >> vn.x >> vn.y >> vn.z;
            normals.push_back(vn);
        }
        else if (type == "f") {
            std::vector<int> face, faceTex, faceNorm;
            char sep;
            int v, t, n;
            
            while (iss >> v) {
                face.push_back(v - 1); // OBJ indices are 1-based
                
                if (iss.peek() == '/') {
                    iss >> sep;
                    if (iss.peek() != '/') {
                        iss >> t;
                        faceTex.push_back(t - 1);
                    }
                    if (iss.peek() == '/') {
                        iss >> sep >> n;
                        faceNorm.push_back(n - 1);
                    }
                }
            }
            
            faces.push_back(face);
            if (!faceTex.empty()) faceTexCoords.push_back(faceTex);
            if (!faceNorm.empty()) faceNormals.push_back(faceNorm);
        }
    }
    return true;
}

bool Model::loadDiffuseTexture(const std::string& filename) {
    return diffuseTexture.loadFromTGA(filename);
}

Vector3<float> Model::calculateFaceNormal(const std::vector<int>& faceIndices) const {
    if (faceIndices.size() < 3) return Vector3<float>(0, 0, 1);
    
    const auto& v0 = vertices[faceIndices[0]];
    const auto& v1 = vertices[faceIndices[1]];
    const auto& v2 = vertices[faceIndices[2]];
    
    Vector3<float> edge1 = v1 - v0;
    Vector3<float> edge2 = v2 - v0;
    return edge1.cross(edge2).normalized();
}

void Model::renderWireframe(Framebuffer& fb, const Vector3<float>& color) {
    for (const auto& face : faces) {
        if (face.size() < 2) continue;
        
        for (size_t i = 0; i < face.size(); i++) {
            size_t j = (i + 1) % face.size();
            const auto& v1 = vertices[face[i]];
            const auto& v2 = vertices[face[j]];
            
            int x1 = static_cast<int>((v1.x + 1) * fb.width / 2);
            int y1 = static_cast<int>((v1.y + 1) * fb.height / 2);
            int x2 = static_cast<int>((v2.x + 1) * fb.width / 2);
            int y2 = static_cast<int>((v2.y + 1) * fb.height / 2);
            
            fb.drawLine(x1, y1, x2, y2, color);
        }
    }
}

void Model::renderSolid(Framebuffer& fb, float near, float far, const Matrix4x4& mvp, const Vector3<float>& color, const Vector3<float>& lightDir) {
    // Note: lightDir should point TOWARDS the light source
    // Common conventions:
    // - Right-handed: (0,0,-1) for light coming from camera
    // - Left-handed: (0,0,1) for light coming from camera
    for (size_t fi = 0; fi < faces.size(); fi++) {
        const auto& face = faces[fi];
        if (face.size() < 3) continue;

        // Get face normal (use provided normals or calculate from geometry)
        Vector3<float> normal;
        // if (fi < faceNormals.size() && faceNormals[fi].size() >= 3) {
        //     normal = (normals[faceNormals[fi][0]] + 
        //              normals[faceNormals[fi][1]] + 
        //              normals[faceNormals[fi][2]]).normalized();
        // } else {
        //     normal = calculateFaceNormal(face);
        // }
        normal = calculateFaceNormal(face);
        // Calculate lighting intensity (dot product with light direction)
        float intensity = normal.dot(lightDir.normalized());

        if (intensity > 0) {
            // 顶点数组
            Vector4<float> clip_coords[3];
            Vector2<float> tex_coords[3];
            Vector3<float> world_coords[3];
            float w_values[3];  // 存储 w 值用于透视校正

            // 变换顶点
            for (int j = 0; j < 3; j++) {
                world_coords[j] = vertices[face[j]];
                Vector4<float> v(world_coords[j], 1.0f);
                clip_coords[j] = mvp * v;
                w_values[j] = clip_coords[j].w;

                if (fi < faceTexCoords.size() && j < faceTexCoords[fi].size()) {
                    const auto& vt = texCoords[faceTexCoords[fi][j]];
                    tex_coords[j] = Vector2<float>(vt.x, vt.y);
                } else {
                    tex_coords[j] = Vector2<float>(0, 0);
                }
            }

            // 裁剪检查（简单版本：丢弃完全在近裁剪面外的三角形）
            if (clip_coords[0].z < -w_values[0] && 
                clip_coords[1].z < -w_values[1] && 
                clip_coords[2].z < -w_values[2]) {
                continue;
            }

            // 透视除法和深度映射
            Vertex vertices[3];
            for (int j = 0; j < 3; j++) {
                if (w_values[j] <= 0) continue; // 防止除以0或负数
                
                // 透视除法
                float invW = 1.0f / w_values[j];
                Vector3<float> ndc(
                    clip_coords[j].x * invW,
                    clip_coords[j].y * invW,
                    clip_coords[j].z * invW
                );

                // 视口变换
                vertices[j].x = (ndc.x + 1.0f) * fb.width * 0.5f;
                vertices[j].y = (ndc.y + 1.0f) * fb.height * 0.5f;
                
                // 将裁剪空间 z 映射到 [0,1] 范围用于深度测试
                float zEye = clip_coords[j].z;  // 视空间 z（负值）
                if (w_values[j] != 0) {
                    // 将视空间 z 映射到 [0,1]，近处为 0，远处为 1
                    vertices[j].z = (1.0f - (near * far / zEye * invW + near) / (far - near)) * 0.5f + 0.5f;
                } else {
                    vertices[j].z = 1.0f; // 默认最远
                }

                // 透视校正纹理坐标
                vertices[j].u = tex_coords[j].x * invW;
                vertices[j].v = tex_coords[j].y * invW;
                vertices[j].w = invW;  // 存储 1/w 用于插值
            }

            Vector3<float> shadedColor = color * intensity;
            fb.drawTriangle(vertices[0], vertices[1], vertices[2], 
                          shadedColor, diffuseTexture);
        }
    }
}
