#pragma once
#include "GameObject.h"

class TankBody : public GameObject
{
public:
    TankBody();
    virtual ~TankBody();

    void Initialize(ComPtr<ID3D12Device> device, UINT count);
    void UpdateInstances(const std::vector<XMFLOAT4X4>& matrices);

    virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj) override;
    void RenderOBBs(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj);

    DirectX::BoundingBox GetWorldAABB() const override { return DirectX::BoundingBox({ 0,0,0 }, { 0,0,0 }); }

    static DirectX::BoundingBox GetSharedLocalAABB() { return sSharedLocalAABB; }

private:
    static DirectX::BoundingBox sSharedLocalAABB;
    std::vector<XMFLOAT4X4>     mCurrentBodyMats;

    ComPtr<ID3D12Resource>   mOBBInstBuf;
    D3D12_VERTEX_BUFFER_VIEW mOBBInstView = {};
};
