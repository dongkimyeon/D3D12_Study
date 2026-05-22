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
    void SetPosition(XMFLOAT3 pos);
    void SetPosition(float x, float y, float z);
    void SetRotation(XMFLOAT3 rot);
    void SetRotation(float pitch, float yaw, float roll);

private:
    std::unique_ptr<HeliBody>  mBody;
    std::unique_ptr<HeliBlade> mBlade;
    std::unique_ptr<HeliTale>  mTail;

    float mBladeAngle = 0.0f;
    float mTailAngle  = 0.0f;
};
