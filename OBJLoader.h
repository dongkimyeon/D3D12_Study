#pragma once
#include "stdafx.h"

struct OBJVertex {
    float x, y, z;       // 위치
    float nx, ny, nz;    // 노멀
    float r, g, b, a;    // 색상
};

class OBJLoader {
public:
    static bool Load(const std::string& filename,
        std::vector<OBJVertex>& vertices,
        std::vector<uint32_t>& indices);

    static bool LoadBinary(const std::string& binPath,
        std::vector<OBJVertex>& vertices,
        std::vector<uint32_t>& indices);
};