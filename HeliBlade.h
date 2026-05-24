#pragma once
#include "GameObject.h"
class HeliBlade : public GameObject
{
public:
    HeliBlade();
    ~HeliBlade();

    void Initialize(ComPtr<ID3D12Device> device) override;
    void Update(float dt) override;
    void Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj) override;
    DirectX::BoundingBox GetWorldAABB() const override { return DirectX::BoundingBox({ 0,0,0 }, { 0,0,0 }); }

private:
    float rotationSpeed;
  
};

