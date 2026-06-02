#include "Tank.h"
#include "TankBody.h"
#include "TankLid.h"
#include "TankBarrel.h"

Tank::Tank()
    : mBody(std::make_unique<TankBody>())
    , mLid(std::make_unique<TankLid>())
    , mBarrel(std::make_unique<TankBarrel>())
{
}

Tank::~Tank() {}

void Tank::Initialize(ComPtr<ID3D12Device> device)
{
    mBody->Initialize(device);
    mLid->Initialize(device);
    mBarrel->Initialize(device);
}

void Tank::Update(float dt)
{
    mBody->Update(dt);
    mLid->Update(dt);
    mBarrel->Update(dt);

    XMFLOAT3 bodyPos    = mBody->GetPosition();
    float     tankScale = 0.04f;
    XMMATRIX  S         = XMMatrixScaling(tankScale, tankScale, tankScale);
    XMMATRIX  R_body    = XMMatrixRotationY(mHeading);
    XMMATRIX  T_body    = XMMatrixTranslation(bodyPos.x, bodyPos.y, bodyPos.z);

    XMMATRIX lid_T = XMMatrixTranslation(mLidOffset.x, mLidOffset.y, mLidOffset.z);
    XMMATRIX lid_R = XMMatrixRotationRollPitchYaw(
        XMConvertToRadians(mLidRotation.x),
        XMConvertToRadians(mLidRotation.y),
        XMConvertToRadians(mLidRotation.z));
    XMFLOAT4X4 lid_mat;
    XMStoreFloat4x4(&lid_mat, lid_T * S * lid_R * R_body * T_body);
    mLid->SetWorldMatrix(lid_mat);

    XMMATRIX barrel_T_pivot_neg    = XMMatrixTranslation(-mBarrelPivot.x, -mBarrelPivot.y, -mBarrelPivot.z);
    XMMATRIX barrel_T_attach_world = XMMatrixTranslation(tankScale * mBarrelOffset.x, tankScale * mBarrelOffset.y, tankScale * mBarrelOffset.z);
    XMMATRIX barrel_R              = XMMatrixRotationRollPitchYaw(
        XMConvertToRadians(mBarrelRotation.x),
        XMConvertToRadians(mLidRotation.y + mBarrelRotation.y),
        XMConvertToRadians(mBarrelRotation.z));
    XMFLOAT4X4 barrel_mat;
    XMStoreFloat4x4(&barrel_mat, barrel_T_pivot_neg * S * barrel_R * barrel_T_attach_world * R_body * T_body);
    mBarrel->SetWorldMatrix(barrel_mat);
}

void Tank::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
    mBody->Render(commandList, view, proj);
    mLid->Render(commandList, view, proj);
    mBarrel->Render(commandList, view, proj);
}

XMFLOAT3 Tank::GetPosition() const
{
    return mBody->GetPosition();
}

void Tank::SetPosition(float x, float y, float z)
{
    mBody->SetPosition(x, y, z);
}

void Tank::SetPosition(XMFLOAT3 pos)
{
    mBody->SetPosition(pos);
}
