#include "stdafx.h"
#include "Missile.h"
#include "Tank.h"

Missile::Missile() {}
Missile::~Missile() {}

void Missile::Initialize(ComPtr<ID3D12Device> device)
{
    GameObject::Initialize(device);
    LoadFromOBJ("sphere.obj", device);
    scale = { 0.3f, 0.3f, 0.3f };
    SetColor(0.6f, 0.7f, 1.f);
}

void Missile::Spawn(XMFLOAT3 pos, XMFLOAT3 dir, float speed)
{
    SetPosition(pos);
    mDirection = dir;
    mSpeed     = speed;
    mLifetime  = 3.f;
}

void Missile::Update(float dt)
{
    if (mLifetime <= 0.f) return;
    mLifetime -= dt;

    if (mTarget && mTarget->IsAlive())
    {
        XMFLOAT3 targetPos = mTarget->GetWorldOBB().Center;
        XMFLOAT3 myPos = GetPosition();
        XMVECTOR desired = XMVector3Normalize(XMLoadFloat3(&targetPos) - XMLoadFloat3(&myPos));
        XMVECTOR current = XMLoadFloat3(&mDirection);
        constexpr float kTurnRate = 5.f;
        XMVECTOR newDir = XMVector3Normalize(XMVectorLerp(current, desired, std::min(kTurnRate * dt, 1.f)));
        XMStoreFloat3(&mDirection, newDir);
    }

    XMFLOAT3 pos = GetPosition();
    pos.x += mDirection.x * mSpeed * dt;
    pos.y += mDirection.y * mSpeed * dt;
    pos.z += mDirection.z * mSpeed * dt;
    SetPosition(pos);

    GameObject::Update(dt);
}

void Missile::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
    if (mLifetime <= 0.f) return;
    GameObject::Render(commandList, view, proj);
}
