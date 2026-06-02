#include "TankBody.h"

TankBody::TankBody() {}
TankBody::~TankBody() {}

void TankBody::Initialize(ComPtr<ID3D12Device> device)
{
    GameObject::Initialize(device);
    LoadFromOBJ("Tank_Body.OBJ", device);
    scale = { 0.04f, 0.04f, 0.04f };
}

void TankBody::Update(float dt)
{
    GameObject::Update(dt);
}

void TankBody::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
    GameObject::Render(commandList, view, proj);
}

DirectX::BoundingOrientedBox TankBody::GetWorldOBB() const
{
    if (vertices.empty()) return DirectX::BoundingOrientedBox({ 0,0,0 }, { 0,0,0 }, { 0,0,0,1 });

    XMFLOAT3 vmin = { FLT_MAX,  FLT_MAX,  FLT_MAX };
    XMFLOAT3 vmax = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
    for (const auto& v : vertices) {
        vmin.x = std::min(vmin.x, v.x); vmax.x = std::max(vmax.x, v.x);
        vmin.y = std::min(vmin.y, v.y); vmax.y = std::max(vmax.y, v.y);
        vmin.z = std::min(vmin.z, v.z); vmax.z = std::max(vmax.z, v.z);
    }
    DirectX::BoundingBox localAABB(
        { (vmin.x + vmax.x) * 0.5f, (vmin.y + vmax.y) * 0.5f, (vmin.z + vmax.z) * 0.5f },
        { (vmax.x - vmin.x) * 0.5f, (vmax.y - vmin.y) * 0.5f, (vmax.z - vmin.z) * 0.5f });

    DirectX::BoundingOrientedBox localOBB;
    DirectX::BoundingOrientedBox::CreateFromBoundingBox(localOBB, localAABB);

    DirectX::BoundingOrientedBox worldOBB;
    localOBB.Transform(worldOBB, XMLoadFloat4x4(&worldMatrix));
    return worldOBB;
}
