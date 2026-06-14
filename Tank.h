#pragma once
#include "stdafx.h"

class Tank
{
public:
    Tank();
    ~Tank();

    void Initialize();
    void Update(float dt);

    XMFLOAT3 GetPosition() const;
    void SetPosition(float x, float y, float z);
    void SetPosition(XMFLOAT3 pos);

    void Hit() { mHitCount++; }
    bool IsAlive() const { return mHitCount < 1; }
    DirectX::BoundingOrientedBox GetWorldOBB() const;

    float mPitch   = 0.0f;
    float mHeading = 0.0f;
    float mRoll    = 0.0f;

    XMFLOAT4X4 mBodyMatrix   = {};
    XMFLOAT4X4 mLidMatrix    = {};
    XMFLOAT4X4 mBarrelMatrix = {};

    XMFLOAT3 mLidOffset      = {  0.f,  67.f,   0.f };
    XMFLOAT3 mLidRotation    = {  0.f,   0.f,   0.f };

    XMFLOAT3 mBarrelOffset   = {  0.f,  45.f, -58.f };
    XMFLOAT3 mBarrelPivot    = {  0.f,   0.f, 125.f };
    XMFLOAT3 mBarrelRotation = {  0.f,   0.f,   0.f };

private:
    XMFLOAT3 mPosition = { 0.f, 0.f, 0.f };
    int       mHitCount = 0;
};
