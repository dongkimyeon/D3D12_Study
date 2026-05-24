#pragma once
#include "GameObject.h"

class Player;

struct ItemMesh
{
    ComPtr<ID3D12Resource>   vb, ib, vbUpload, ibUpload;
    D3D12_VERTEX_BUFFER_VIEW vbView = {};
    D3D12_INDEX_BUFFER_VIEW  ibView = {};
    UINT                     indexCount  = 0;
    D3D12_RESOURCE_STATES    vbState = D3D12_RESOURCE_STATE_COPY_DEST;
    D3D12_RESOURCE_STATES    ibState = D3D12_RESOURCE_STATE_COPY_DEST;
    bool                     vbDirty = false;
    bool                     ibDirty = false;
    DirectX::BoundingBox     localAABB;

    ComPtr<ID3D12Resource>   instanceBuffer;
    D3D12_VERTEX_BUFFER_VIEW instanceView = {};
    UINT                     maxInstances = 0;
};

class Item : public GameObject
{
public:
    virtual void Update(float dt) override;
    virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList,
                        XMMATRIX view, XMMATRIX proj) override;

    virtual void OnPickup(Player* player) = 0;

    bool IsActive() const  { return mActive; }
    void SetInactive()     { mActive = false; }

    virtual DirectX::BoundingBox GetWorldAABB() const override;

protected:
    XMFLOAT4             mColor     = { 1, 1, 1, 1 };
    bool                 mActive    = true;
    DirectX::BoundingBox mLocalAABB = {};

    virtual ItemMesh& GetMesh() = 0;

    // 각 서브클래스의 LoadSharedMesh에서 공통으로 호출
    static void LoadMesh(ComPtr<ID3D12Device> device,
                         const std::string& filename, ItemMesh& m);
    static void UnloadMesh(ItemMesh& m);

    // 인스턴싱 배치 렌더 (같은 메쉬를 공유하는 아이템 목록 전달)
    static void RenderBatch(ComPtr<ID3D12GraphicsCommandList>& commandList,
                             ItemMesh& mesh, const std::vector<Item*>& items);
};
