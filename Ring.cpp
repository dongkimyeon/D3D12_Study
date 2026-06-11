#include "stdafx.h"
#include "Ring.h"

void Ring::Initialize(ComPtr<ID3D12Device> device)
{
    GameObject::Initialize(device);
    LoadFromOBJ("Ring.OBJ", device);

    // Ring.OBJ outer radius ~385 units -> scale to ~27 unit radius (54 diameter)
    BakeScale(0.07f, 0.07f, 0.07f);

    // Recompute local AABB after bake
    if (!vertices.empty()) {
        XMFLOAT3 vmin = {  FLT_MAX,  FLT_MAX,  FLT_MAX };
        XMFLOAT3 vmax = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
        for (const auto& v : vertices) {
            vmin.x = std::min(vmin.x, v.x); vmax.x = std::max(vmax.x, v.x);
            vmin.y = std::min(vmin.y, v.y); vmax.y = std::max(vmax.y, v.y);
            vmin.z = std::min(vmin.z, v.z); vmax.z = std::max(vmax.z, v.z);
        }
        mLocalAABB = DirectX::BoundingBox(
            { (vmin.x+vmax.x)*0.5f, (vmin.y+vmax.y)*0.5f, (vmin.z+vmax.z)*0.5f },
            { (vmax.x-vmin.x)*0.5f, (vmax.y-vmin.y)*0.5f, (vmax.z-vmin.z)*0.5f });
    }

    // Gold color
    SetColor(1.0f, 0.84f, 0.0f);
}

bool Ring::CheckCollision(const XMFLOAT3& heliPos) const
{
    XMFLOAT3 ringPos = GetPosition();
    float dx = heliPos.x - ringPos.x;
    float dy = heliPos.y - ringPos.y;
    float dz = heliPos.z - ringPos.z;
    return (dx*dx + dy*dy + dz*dz) < (kTriggerRadius * kTriggerRadius);
}
