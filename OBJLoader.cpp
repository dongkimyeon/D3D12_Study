#include "OBJLoader.h"
#include <fstream>
#include <sstream>
#include <iostream>

bool OBJLoader::Load(const std::string& filename,
    std::vector<OBJVertex>& vertices,
    std::vector<uint16_t>& indices) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Failed to open OBJ file: " << filename << std::endl;
        return false;
    }

    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<uint16_t> posIndices, normIndices;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {
            // 정점 위치
            float x, y, z;
            iss >> x >> y >> z;
            positions.push_back(x);
            positions.push_back(y);
            positions.push_back(z);
        }
        else if (prefix == "vn") {
            // 정점 노멀
            float nx, ny, nz;
            iss >> nx >> ny >> nz;
            normals.push_back(nx);
            normals.push_back(ny);
            normals.push_back(nz);
        }
        else if (prefix == "f") {
            // 면 (삼각형만 지원)
            std::string v1, v2, v3;
            iss >> v1 >> v2 >> v3;

            for (const auto& vStr : { v1, v2, v3 }) {
                std::istringstream viss(vStr);
                std::string posIdx, texIdx, normIdx;

                std::getline(viss, posIdx, '/');
                std::getline(viss, texIdx, '/');
                std::getline(viss, normIdx, '/');

                posIndices.push_back(static_cast<uint16_t>(std::stoi(posIdx) - 1));
                if (!normIdx.empty()) {
                    normIndices.push_back(static_cast<uint16_t>(std::stoi(normIdx) - 1));
                }
            }
        }
    }

    // 정점 데이터 구성
    vertices.clear();
    indices.clear();

    for (size_t i = 0; i < posIndices.size(); ++i) {
        OBJVertex vertex = {};

        uint16_t posIdx = posIndices[i];
        vertex.x = positions[posIdx * 3 + 0];
        vertex.y = positions[posIdx * 3 + 1];
        vertex.z = positions[posIdx * 3 + 2];

        if (!normIndices.empty()) {
            uint16_t normIdx = normIndices[i];
            vertex.nx = normals[normIdx * 3 + 0];
            vertex.ny = normals[normIdx * 3 + 1];
            vertex.nz = normals[normIdx * 3 + 2];
        }

        vertex.r = 0.8f;
        vertex.g = 0.8f;
        vertex.b = 0.8f;
        vertex.a = 1.0f;

        vertices.push_back(vertex);
        indices.push_back(static_cast<uint16_t>(i));
    }

    std::cout << "Loaded " << vertices.size() << " vertices, "
        << indices.size() / 3 << " triangles" << std::endl;

    return true;
}