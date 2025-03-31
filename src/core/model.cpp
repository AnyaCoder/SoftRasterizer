#include "core/model.h"
#include "core/framebuffer.h"
#include <fstream>
#include <sstream>
#include <string>

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
        else if (type == "f") {
            std::vector<int> face;
            int idx;
            while (iss >> idx) {
                face.push_back(idx - 1); // OBJ indices are 1-based
                // Skip texture/normal indices if present
                if (iss.peek() == '/') {
                    iss.ignore();
                    if (iss.peek() != '/') iss >> idx; // texture index
                    if (iss.peek() == '/') {
                        iss.ignore();
                        iss >> idx; // normal index
                    }
                }
            }
            faces.push_back(face);
        }
    }
    return true;
}

void Model::renderWireframe(Framebuffer& fb, const Vector3<float>& color) {
    for (const auto& face : faces) {
        if (face.size() < 2) continue;
        
        // Draw lines between consecutive vertices
        for (size_t i = 0; i < face.size(); i++) {
            size_t j = (i + 1) % face.size();
            const auto& v1 = vertices[face[i]];
            const auto& v2 = vertices[face[j]];
            
            // Simple projection - just drop Z coordinate for now
            int x1 = static_cast<int>((v1.x + 1) * fb.width / 2);
            int y1 = fb.height - static_cast<int>((v1.y + 1) * fb.height / 2);
            int x2 = static_cast<int>((v2.x + 1) * fb.width / 2);
            int y2 = fb.height - static_cast<int>((v2.y + 1) * fb.height / 2);
            
            fb.drawLine(x1, y1, x2, y2, color);
        }
    }
}
