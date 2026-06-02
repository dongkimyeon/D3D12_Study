#include "TankBody.h"

TankBody::TankBody() {}
TankBody::~TankBody() {}

void TankBody::Initialize(ComPtr<ID3D12Device> device)
{
    GameObject::Initialize(device);
    LoadFromOBJ("Tank_Body.OBJ", device);
    scale = { 0.04f, 0.04f, 0.04f };
    SetColor(1.f, 0.6f, 0.6f);
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
    DirectX::BoundingOrientedBox localOBB;
    DirectX::BoundingOrientedBox::CreateFromBoundingBox(localOBB, mLocalAABB);

    DirectX::BoundingOrientedBox worldOBB;
    localOBB.Transform(worldOBB, XMLoadFloat4x4(&worldMatrix));
    return worldOBB;
}
