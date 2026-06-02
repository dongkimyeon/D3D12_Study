#include "TankBarrel.h"

TankBarrel::TankBarrel() {}
TankBarrel::~TankBarrel() {}

void TankBarrel::Initialize(ComPtr<ID3D12Device> device)
{
    GameObject::Initialize(device);
    LoadFromOBJ("Tank_Barrel.OBJ", device);
    scale = { 0.05f, 0.05f, 0.05f };
    SetColor(1.f, 0.6f, 0.6f);
}

void TankBarrel::Update(float dt)
{
    GameObject::Update(dt);
}

void TankBarrel::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
    GameObject::Render(commandList, view, proj);
}
