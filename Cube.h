#pragma once
#include "GameObject.h"

class Cube : public GameObject
{
public:
    Cube();
    virtual ~Cube();

    virtual void Initialize(ComPtr<ID3D12Device> device) override;
    virtual void Update(float dt) override;
    virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj) override;

    // AABB auto-updated every frame from position + scale
    const DirectX::BoundingBox& GetAABB() const { return mAABB; }

private:
    XMFLOAT4 mColor = { 1,1,1,1 };
    DirectX::BoundingBox mAABB;
    void UpdateAABB();

    // ---- Shared mesh: one GPU VB+IB reused by every Cube instance ----
    static void LoadSharedMesh(ComPtr<ID3D12Device> device);
    static void UnloadSharedMesh();

    static ComPtr<ID3D12Resource> sVB;          // default heap (GPU)
    static ComPtr<ID3D12Resource> sIB;
    static ComPtr<ID3D12Resource> sVBUpload;    // upload heap (staging)
    static ComPtr<ID3D12Resource> sIBUpload;
    static D3D12_VERTEX_BUFFER_VIEW sVbView;
    static D3D12_INDEX_BUFFER_VIEW  sIbView;
    static UINT    sIndexCount;
    static int     sRefCount;                   // how many Cube instances exist
    static D3D12_RESOURCE_STATES sVBState;
    static D3D12_RESOURCE_STATES sIBState;
    static bool    sVBDirty;
    static bool    sIBDirty;
};
