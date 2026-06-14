#include "OBJLoader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>

bool OBJLoader::Load(const std::string& filename,
    std::vector<OBJVertex>& vertices,
    std::vector<uint32_t>& indices) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Failed to open OBJ file: " << filename << std::endl;
        return false;
    }

    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<uint32_t> posIndices, normIndices;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {
    
            float x, y, z;
            iss >> x >> y >> z;
            positions.push_back(x);
            positions.push_back(y);
            positions.push_back(z);
        }
        else if (prefix == "vn") {
         
            float nx, ny, nz;
            iss >> nx >> ny >> nz;
            normals.push_back(nx);
            normals.push_back(ny);
            normals.push_back(nz);
        }
        else if (prefix == "f") {
           
            std::string v1, v2, v3;
            iss >> v1 >> v2 >> v3;

            for (const auto& vStr : { v1, v2, v3 }) {
                std::istringstream viss(vStr);
                std::string posIdx, texIdx, normIdx;

                std::getline(viss, posIdx, '/');
                std::getline(viss, texIdx, '/');
                std::getline(viss, normIdx, '/');

                posIndices.push_back(static_cast<uint32_t>(std::stoi(posIdx) - 1));
                if (!normIdx.empty()) {
                    normIndices.push_back(static_cast<uint32_t>(std::stoi(normIdx) - 1));
                }
            }
        }
    }

    vertices.clear();
    indices.clear();

    for (size_t i = 0; i < posIndices.size(); ++i) {
        OBJVertex vertex = {};

        uint32_t posIdx = posIndices[i];
        vertex.x = positions[posIdx * 3 + 0];
        vertex.y = positions[posIdx * 3 + 1];
        vertex.z = positions[posIdx * 3 + 2];

        if (!normIndices.empty()) {
            uint32_t normIdx = normIndices[i];
            vertex.nx = normals[normIdx * 3 + 0];
            vertex.ny = normals[normIdx * 3 + 1];
            vertex.nz = normals[normIdx * 3 + 2];
        }

        vertex.r = 0.8f;
        vertex.g = 0.8f;
        vertex.b = 0.8f;
        vertex.a = 1.0f;

        vertices.push_back(vertex);
        indices.push_back(static_cast<uint32_t>(i));
    }

    if (normIndices.empty()) {
        for (size_t i = 0; i + 2 < vertices.size(); i += 3) {
            float ax = vertices[i + 1].x - vertices[i].x;
            float ay = vertices[i + 1].y - vertices[i].y;
            float az = vertices[i + 1].z - vertices[i].z;
            float bx = vertices[i + 2].x - vertices[i].x;
            float by = vertices[i + 2].y - vertices[i].y;
            float bz = vertices[i + 2].z - vertices[i].z;

            float nx = ay * bz - az * by;
            float ny = az * bx - ax * bz;
            float nz = ax * by - ay * bx;

            float len = sqrtf(nx * nx + ny * ny + nz * nz);
            if (len > 1e-6f) { nx /= len; ny /= len; nz /= len; }

            vertices[i].nx     = nx; vertices[i].ny     = ny; vertices[i].nz     = nz;
            vertices[i + 1].nx = nx; vertices[i + 1].ny = ny; vertices[i + 1].nz = nz;
            vertices[i + 2].nx = nx; vertices[i + 2].ny = ny; vertices[i + 2].nz = nz;
        }
    }

	PrintLog(LogColor::BLUE, "[OBJLoader] Loaded " + std::to_string(vertices.size()) + " vertices, " +
        std::to_string(indices.size() / 3) + " triangles");

    if (!vertices.empty()) {
        float minX =  FLT_MAX, minY =  FLT_MAX, minZ =  FLT_MAX;
        float maxX = -FLT_MAX, maxY = -FLT_MAX, maxZ = -FLT_MAX;

        for (const auto& v : vertices) {
            if (v.x < minX) minX = v.x;  if (v.x > maxX) maxX = v.x;
            if (v.y < minY) minY = v.y;  if (v.y > maxY) maxY = v.y;
            if (v.z < minZ) minZ = v.z;  if (v.z > maxZ) maxZ = v.z;
        }

        float cx = (minX + maxX) * 0.5f;
        float cy = (minY + maxY) * 0.5f;
        float cz = (minZ + maxZ) * 0.5f;

        for (auto& v : vertices) {
            v.x -= cx;
            v.y -= cy;
            v.z -= cz;
        }
    }

    return true;
}

bool OBJLoader::LoadBinary(const std::string& binPath,
    std::vector<OBJVertex>& vertices,
    std::vector<uint32_t>& indices)
{
    FILE* f = nullptr;
    fopen_s(&f, binPath.c_str(), "rb");
    if (!f) return false;

    uint32_t vCount, iCount;
    fread(&vCount, sizeof(uint32_t), 1, f);
    fread(&iCount, sizeof(uint32_t), 1, f);

    vertices.resize(vCount);
    indices.resize(iCount);

    fread(vertices.data(), sizeof(OBJVertex), vCount, f);
    fread(indices.data(),  sizeof(uint32_t),  iCount, f);

    fclose(f);
    PrintLog(LogColor::BLUE, "[OBJLoader] Loaded binary: " + binPath +
        " (" + std::to_string(vCount) + " vertices, " + std::to_string(iCount / 3) + " triangles)");
    return true;
}