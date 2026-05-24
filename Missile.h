#pragma once
#include "GameObject.h"

class Missile : public GameObject
{
public:
    Missile();
    virtual ~Missile();

    virtual void Initialize(ComPtr<ID3D12Device> device) override;
    virtual void Update(float dt) override;
    virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj) override;

    void Spawn(XMFLOAT3 pos, XMFLOAT3 dir, float speed = 120.f);
    bool IsDead() const { return mLifetime <= 0.f; }

private:
    XMFLOAT3 mDirection = {};
    float    mSpeed     = 120.f;
    float    mLifetime  = 0.f;
};
