#pragma once
#include "GameObject.h"

class TankBody : public GameObject
{
public:
    TankBody();
    virtual ~TankBody();

    virtual void Initialize(ComPtr<ID3D12Device> device) override;
    virtual void Update(float dt) override;
    virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj) override;

    virtual DirectX::BoundingOrientedBox GetWorldOBB() const override;
    virtual bool UseOBB() const override { return true; }
};
