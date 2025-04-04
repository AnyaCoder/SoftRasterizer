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

void Model::renderSolid(Framebuffer& fb, const Vector3<float>& color, const Vector3<float>& lightDir) {
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
            // Scale color and convert to 0-255 range
            Vector3<float> shadedColor = color * intensity;
            
            // Project vertices to screen space
            Vector2<int> screen_coords[3];
            Vector3<float> world_coords[3];
            Vector2<float> tex_coords[3];
            
            for (int j = 0; j < 3; j++) {
                const auto& v = vertices[face[j]];
                // Ensure vertex coordinates are in [-1,1] range
                float x = std::max(-1.0f, std::min(1.0f, v.x));
                float y = std::max(-1.0f, std::min(1.0f, v.y));
                screen_coords[j] = Vector2<int>(
                    static_cast<int>((x+1)*fb.width/2), 
                    static_cast<int>((y+1)*fb.height/2)
                );
                world_coords[j] = v;
                
                // Get texture coordinates if available
                if (fi < faceTexCoords.size() && j < faceTexCoords[fi].size()) {
                    const auto& vt = texCoords[faceTexCoords[fi][j]];
                    tex_coords[j] = Vector2<float>(vt.x, vt.y);
                } else {
                    tex_coords[j] = Vector2<float>(0, 0);
                }
            }

            // Draw textured triangle
            Vertex v0(screen_coords[0].x, screen_coords[0].y, world_coords[0].z, tex_coords[0].x, tex_coords[0].y);
            Vertex v1(screen_coords[1].x, screen_coords[1].y, world_coords[1].z, tex_coords[1].x, tex_coords[1].y);
            Vertex v2(screen_coords[2].x, screen_coords[2].y, world_coords[2].z, tex_coords[2].x, tex_coords[2].y);
            
            // Use white color multiplied by intensity for lighting
            fb.drawTriangle(v0, v1, v2, shadedColor, diffuseTexture);
        }
    }
}
