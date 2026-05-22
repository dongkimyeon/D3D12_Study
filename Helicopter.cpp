#include "Helicopter.h"
#include "HeliBody.h"
#include "HeliBlade.h"
#include "HeliTale.h"

Helicopter::Helicopter()
    : mBody(std::make_unique<HeliBody>())
    , mBlade(std::make_unique<HeliBlade>())
    , mTail(std::make_unique<HeliTale>())
{
}

Helicopter::~Helicopter()
{
}

void Helicopter::Initialize(ComPtr<ID3D12Device> device)
{
    mBody->Initialize(device);
    mBody->BakeRotationX(-90.f);

    mBlade->Initialize(device);
    mBlade->BakeRotationX(-90.f);

    mTail->Initialize(device);
    mTail->BakeRotationX(-90.f);
}

void Helicopter::Update(float dt)
{
    mBladeAngle += 1000.0f * dt;
    mTailAngle  +=  800.0f * dt;

    XMFLOAT3 bodyPos = mBody->GetPosition();
    XMFLOAT3 bodyRot = mBody->GetRotation();
    XMMATRIX mBodyRot = XMMatrixRotationRollPitchYaw(bodyRot.x, bodyRot.y, bodyRot.z);

    // 블레이드: 몸통 Y축 기준 회전
    mBlade->SetRotation(bodyRot.x, bodyRot.y + XMConvertToRadians(mBladeAngle), bodyRot.z);

    XMVECTOR bladeWorldOffset = XMVector3TransformNormal(XMVectorSet(2.6f, 0.8f, 0.0f, 0.0f), mBodyRot);
    XMFLOAT3 bladePos;
    XMStoreFloat3(&bladePos, XMLoadFloat3(&bodyPos) + bladeWorldOffset);
    mBlade->SetPosition(bladePos);

    // 꼬리 로터: 몸통 Z축 기준 회전
    mTail->SetRotation(bodyRot.x, bodyRot.y, bodyRot.z + XMConvertToRadians(mTailAngle));

    XMVECTOR tailWorldOffset = XMVector3TransformNormal(XMVectorSet(-6.4f, 0.7f, -0.3f, 0.0f), mBodyRot);
    XMFLOAT3 tailPos;
    XMStoreFloat3(&tailPos, XMLoadFloat3(&bodyPos) + tailWorldOffset);
    mTail->SetPosition(tailPos);

    mBody->Update(dt);
    mBlade->Update(dt);
    mTail->Update(dt);
}

void Helicopter::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
    mBody->Render(commandList, view, proj);
    mBlade->Render(commandList, view, proj);
    mTail->Render(commandList, view, proj);
}

XMFLOAT3 Helicopter::GetPosition() const
{
    return mBody->GetPosition();
}

XMFLOAT3 Helicopter::GetRotation() const
{
    return mBody->GetRotation();
}

void Helicopter::SetPosition(XMFLOAT3 pos)
{
    mBody->SetPosition(pos);
}

void Helicopter::SetPosition(float x, float y, float z)
{
    mBody->SetPosition(x, y, z);
}

void Helicopter::SetRotation(XMFLOAT3 rot)
{
    mBody->SetRotation(rot);
}

void Helicopter::SetRotation(float pitch, float yaw, float roll)
{
    mBody->SetRotation(pitch, yaw, roll);
}
