#pragma once
#include "stdafx.h"
#include <memory>

class HeliBody;
class HeliBlade;
class HeliTale;

class Helicopter
{
public:
    Helicopter();
    ~Helicopter();

    void Initialize(ComPtr<ID3D12Device> device);
    void Update(float dt);
    void Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj);

    XMFLOAT3 GetPosition() const;
    XMFLOAT3 GetRotation() const;
    float     GetHeading()   const { return mHeading; }
    float     GetTiltPitch() const { return mTiltPitch; }
    float     GetTiltRoll()  const { return mTiltRoll; }
    void SetPosition(XMFLOAT3 pos);
    void SetPosition(float x, float y, float z);
    void SetRotation(XMFLOAT3 rot);
    void SetRotation(float pitch, float yaw, float roll);

    void GetFireData(XMFLOAT3& pos1, XMFLOAT3& pos2, XMFLOAT3& dir) const;

    XMFLOAT3 mMissileOffset1 = { -1.f, 0.f, 2.f };
    XMFLOAT3 mMissileOffset2 = {  1.f, 0.f, 2.f };

private:
    std::unique_ptr<HeliBody>  mBody;
    std::unique_ptr<HeliBlade> mBlade;
    std::unique_ptr<HeliTale>  mTail;

    float mBladeAngle = 0.0f;
    float mTailAngle  = 0.0f;

    float    mHeading      = 0.0f;
    float    mTiltPitch    = 0.0f;
    float    mTiltRoll     = 0.0f;
    float    mMoveSpeed    = 50.0f;
    float    mAcceleration = 2.5f;
    XMFLOAT3 mVelocity     = { 0.f, 0.f, 0.f };
    float mMouseSensitivity = 0.003f;
    POINT mPrevMousePos = {};
    bool  mFirstMouse   = true;
};
