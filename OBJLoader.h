#pragma once
#include "stdafx.h"

struct OBJVertex {
    float x, y, z;
    float nx, ny, nz;
    float r, g, b, a;
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