#pragma once
#include "GameObject.h"

class TankBarrel : public GameObject
{
public:
    TankBarrel();
    virtual ~TankBarrel();

    void Initialize(ComPtr<ID3D12Device> device, UINT count);
    void UpdateInstances(const std::vector<XMFLOAT4X4>& matrices);

    virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj) override;

    DirectX::BoundingBox GetWorldAABB() const override { return DirectX::BoundingBox({ 0,0,0 }, { 0,0,0 }); }
};
