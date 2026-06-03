#include "Tank.h"
#include "TankBody.h"

Tank::Tank() {}
Tank::~Tank() {}

void Tank::Initialize() {}

DirectX::BoundingOrientedBox Tank::GetWorldOBB() const
{
    DirectX::BoundingOrientedBox localOBB;
    DirectX::BoundingOrientedBox::CreateFromBoundingBox(localOBB, TankBody::GetSharedLocalAABB());

    DirectX::BoundingOrientedBox worldOBB;
    localOBB.Transform(worldOBB, XMLoadFloat4x4(&mBodyMatrix));
    return worldOBB;
}

void Tank::Update(float dt)
{
    if (!IsAlive()) return;

    float    tankScale = 0.04f;
    XMMATRIX S         = XMMatrixScaling(tankScale, tankScale, tankScale);
    XMMATRIX R_body    = XMMatrixRotationRollPitchYaw(mPitch, mHeading, mRoll);
    XMMATRIX T_body    = XMMatrixTranslation(mPosition.x, mPosition.y, mPosition.z);

    XMStoreFloat4x4(&mBodyMatrix, S * R_body * T_body);

    XMMATRIX lid_T            = XMMatrixTranslation(mLidOffset.x, mLidOffset.y, mLidOffset.z);
    XMMATRIX lid_R_pitch_roll = XMMatrixRotationRollPitchYaw(XMConvertToRadians(mLidRotation.x), 0.f, XMConvertToRadians(mLidRotation.z));
    XMMATRIX lid_R_yaw        = XMMatrixRotationY(XMConvertToRadians(mLidRotation.y));
    XMStoreFloat4x4(&mLidMatrix, lid_T * S * lid_R_pitch_roll * lid_R_yaw * R_body * T_body);

    XMMATRIX barrel_T_pivot_neg    = XMMatrixTranslation(-mBarrelPivot.x, -mBarrelPivot.y, -mBarrelPivot.z);
    XMMATRIX barrel_T_attach_world = XMMatrixTranslation(tankScale * mBarrelOffset.x, tankScale * mBarrelOffset.y, tankScale * mBarrelOffset.z);
    XMMATRIX barrel_R_pitch        = XMMatrixRotationRollPitchYaw(XMConvertToRadians(mBarrelRotation.x), 0.f, XMConvertToRadians(mBarrelRotation.z));
    XMMATRIX barrel_R_yaw          = XMMatrixRotationY(XMConvertToRadians(mLidRotation.y + mBarrelRotation.y));
    XMStoreFloat4x4(&mBarrelMatrix, barrel_T_pivot_neg * S * barrel_R_pitch * barrel_T_attach_world * barrel_R_yaw * R_body * T_body);
}

XMFLOAT3 Tank::GetPosition() const            { return mPosition; }
void Tank::SetPosition(float x, float y, float z) { mPosition = { x, y, z }; }
void Tank::SetPosition(XMFLOAT3 pos)          { mPosition = pos; }
