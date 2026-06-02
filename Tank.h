#pragma once
#include "stdafx.h"
#include <memory>

class TankBody;
class TankLid;
class TankBarrel;

class Tank
{
public:
    Tank();
    ~Tank();

    void Initialize(ComPtr<ID3D12Device> device);
    void Update(float dt);
    void Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj);

    XMFLOAT3 GetPosition() const;
    void SetPosition(float x, float y, float z);
    void SetPosition(XMFLOAT3 pos);

    float GetHeading() const { return mHeading; }

    // OBJ local space 기준 (scale 독립적)
    XMFLOAT3 mLidOffset      = {  0.f,  67.f,   0.f  };
    XMFLOAT3 mLidRotation    = {  0.f,   0.f,   0.f  };

    XMFLOAT3 mBarrelOffset   = {  0.f,  45.f, -58.f  };
    XMFLOAT3 mBarrelPivot    = {  0.f,   0.f, 125.f  };
    XMFLOAT3 mBarrelRotation = {  0.f,   0.f,   0.f  };

private:
    std::unique_ptr<TankBody>   mBody;
    std::unique_ptr<TankLid>    mLid;
    std::unique_ptr<TankBarrel> mBarrel;

    float mHeading = 0.0f;
};
