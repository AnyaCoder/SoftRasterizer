#include "core/model.h"
#include "core/framebuffer.h"
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>


const Vector3<float>& Model::getVertex(int index) const {
    // Add bounds checking if needed
    return vertices[index];
}
const Vector3<float>& Model::getNormal(int index) const {
     // Add bounds checking if needed
    return normals[index];
}
const Vector2<float>& Model::getUV(int index) const {
     // Add bounds checking if needed
    return uvs[index];
}
const Model::Face& Model::getFace(int index) const {
     // Add bounds checking if needed
    return faces[index];
}

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
            vec3f v;
            iss >> v.x >> v.y >> v.z;
            vertices.push_back(v);
        }
        else if (type == "vt") {
            vec2f vt;
            iss >> vt.x >> vt.y;
            uvs.push_back(vt);
        }
        else if (type == "vn") {
            vec3f vn;
            iss >> vn.x >> vn.y >> vn.z;
            normals.push_back(vn);
        }
        else if (type == "f") {
            Face face;
            char sep;
            int v, t, n, i = 0;
            
            while (iss >> v) {
                face.vertIndex[i] = --v;
                
                if (iss.peek() == '/') {
                    iss >> sep;
                    if (iss.peek() != '/') {
                        iss >> t;
                        face.uvIndex[i] = --t;
                    }
                    if (iss.peek() == '/') {
                        iss >> sep >> n;
                        face.normIndex[i] = --n;
                    }
                }
                i++;
            }
            
            faces.push_back(face);
        }
    }
    return true;
}

bool Model::loadDiffuseTexture(const std::string& filename) {
    return diffuseTexture.loadFromTGA(filename);
}

