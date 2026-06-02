#include "TankLid.h"

TankLid::TankLid() {}
TankLid::~TankLid() {}

void TankLid::Initialize(ComPtr<ID3D12Device> device)
{
    GameObject::Initialize(device);
    LoadFromOBJ("Tank_Lid.OBJ", device);
    scale = { 0.05f, 0.05f, 0.05f };
    SetColor(1.f, 0.6f, 0.6f);
}

void TankLid::Update(float dt)
{
    GameObject::Update(dt);
}

void TankLid::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
    GameObject::Render(commandList, view, proj);
}
