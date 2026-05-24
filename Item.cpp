#include "Item.h"

void Item::Update(float dt)
{
    if (mActive)
        rotation.y += 2.0f * dt;
    GameObject::Update(dt);
}

void Item::Render(ComPtr<ID3D12GraphicsCommandList>& commandList,
                  XMMATRIX view, XMMATRIX proj)
{
    // 메쉬 드로우는 RenderBatch에서 처리 — 여기서는 AABB 디버그만
    if (!mActive) return;
    if (sShowAABB)
        RenderAABB(commandList, view, proj);
}

void Item::RenderBatch(ComPtr<ID3D12GraphicsCommandList>& commandList,
                       ItemMesh& m, const std::vector<Item*>& items)
{
    if (m.indexCount == 0 || !m.instanceBuffer) return;

    auto flush = [&](ComPtr<ID3D12Resource>& gpu, ComPtr<ID3D12Resource>& upload,
        D3D12_RESOURCE_STATES& state, D3D12_RESOURCE_STATES target,
        UINT64 size, bool& dirty)
    {
        if (!dirty) return;
        if (state != D3D12_RESOURCE_STATE_COPY_DEST) {
            D3D12_RESOURCE_BARRIER b = {};
            b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            b.Transition.pResource   = gpu.Get();
            b.Transition.StateBefore = state;
            b.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
            b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            commandList->ResourceBarrier(1, &b);
        }
        commandList->CopyBufferRegion(gpu.Get(), 0, upload.Get(), 0, size);
        D3D12_RESOURCE_BARRIER b = {};
        b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition.pResource   = gpu.Get();
        b.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        b.Transition.StateAfter  = target;
        b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &b);
        state = target; dirty = false;
    };
    flush(m.vb, m.vbUpload, m.vbState, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m.vbView.SizeInBytes, m.vbDirty);
    flush(m.ib, m.ibUpload, m.ibState, D3D12_RESOURCE_STATE_INDEX_BUFFER,               m.ibView.SizeInBytes, m.ibDirty);

    UINT count = 0;
    void* mapped;
    m.instanceBuffer->Map(0, nullptr, &mapped);
    InstanceData* ptr = reinterpret_cast<InstanceData*>(mapped);
    for (auto* item : items)
    {
        if (!item->mActive) continue;
        if (count >= m.maxInstances) break;
        ptr[count].world = item->worldMatrix;
        ptr[count].color = item->mColor;
        ++count;
    }
    m.instanceBuffer->Unmap(0, nullptr);

    if (count == 0) return;

    commandList->IASetVertexBuffers(0, 1, &m.vbView);
    commandList->IASetVertexBuffers(1, 1, &m.instanceView);
    commandList->IASetIndexBuffer(&m.ibView);
    commandList->DrawIndexedInstanced(m.indexCount, count, 0, 0, 0);
}

DirectX::BoundingBox Item::GetWorldAABB() const
{
    DirectX::BoundingBox world;
    mLocalAABB.Transform(world, XMLoadFloat4x4(&worldMatrix));
    return world;
}

void Item::LoadMesh(ComPtr<ID3D12Device> device,
                    const std::string& filename, ItemMesh& m)
{
    std::vector<OBJVertex> verts;
    std::vector<uint16_t>  inds;
    OBJLoader::Load(filename, verts, inds);

    // BakeScale 0.01 + 흰색 (색상은 per-instance mColor로 처리)
    XMFLOAT3 vmin = {  FLT_MAX,  FLT_MAX,  FLT_MAX };
    XMFLOAT3 vmax = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
    for (auto& v : verts)
    {
        v.x *= 0.01f; v.y *= 0.01f; v.z *= 0.01f;
        v.r = v.g = v.b = v.a = 1.0f;
        vmin.x = std::min(vmin.x, v.x); vmax.x = std::max(vmax.x, v.x);
        vmin.y = std::min(vmin.y, v.y); vmax.y = std::max(vmax.y, v.y);
        vmin.z = std::min(vmin.z, v.z); vmax.z = std::max(vmax.z, v.z);
    }
    m.localAABB = DirectX::BoundingBox(
        { (vmin.x + vmax.x) * 0.5f, (vmin.y + vmax.y) * 0.5f, (vmin.z + vmax.z) * 0.5f },
        { (vmax.x - vmin.x) * 0.5f, (vmax.y - vmin.y) * 0.5f, (vmax.z - vmin.z) * 0.5f });

    D3D12_HEAP_PROPERTIES upload = { D3D12_HEAP_TYPE_UPLOAD };
    D3D12_HEAP_PROPERTIES def    = { D3D12_HEAP_TYPE_DEFAULT };

    UINT vbSize = (UINT)(verts.size() * sizeof(OBJVertex));
    D3D12_RESOURCE_DESC vRes = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, vbSize,
        1, 1, 1, DXGI_FORMAT_UNKNOWN, {1,0},
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };

    device->CreateCommittedResource(&upload, D3D12_HEAP_FLAG_NONE, &vRes,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m.vbUpload));
    device->CreateCommittedResource(&def, D3D12_HEAP_FLAG_NONE, &vRes,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m.vb));

    void* mapped;
    m.vbUpload->Map(0, nullptr, &mapped);
    memcpy(mapped, verts.data(), vbSize);
    m.vbUpload->Unmap(0, nullptr);

    m.vbView.BufferLocation = m.vb->GetGPUVirtualAddress();
    m.vbView.StrideInBytes  = sizeof(OBJVertex);
    m.vbView.SizeInBytes    = vbSize;
    m.vbState = D3D12_RESOURCE_STATE_COPY_DEST;
    m.vbDirty = true;

    UINT ibSize = (UINT)(inds.size() * sizeof(uint16_t));
    D3D12_RESOURCE_DESC iRes = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, ibSize,
        1, 1, 1, DXGI_FORMAT_UNKNOWN, {1,0},
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };

    device->CreateCommittedResource(&upload, D3D12_HEAP_FLAG_NONE, &iRes,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m.ibUpload));
    device->CreateCommittedResource(&def, D3D12_HEAP_FLAG_NONE, &iRes,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m.ib));

    m.ibUpload->Map(0, nullptr, &mapped);
    memcpy(mapped, inds.data(), ibSize);
    m.ibUpload->Unmap(0, nullptr);

    m.ibView.BufferLocation = m.ib->GetGPUVirtualAddress();
    m.ibView.Format         = DXGI_FORMAT_R16_UINT;
    m.ibView.SizeInBytes    = ibSize;
    m.ibState = D3D12_RESOURCE_STATE_COPY_DEST;
    m.ibDirty = true;

    m.indexCount = (UINT)inds.size();

    // 인스턴스 버퍼 (최대 100개, 업로드 힙)
    m.maxInstances = 100;
    UINT instBufSize = m.maxInstances * sizeof(InstanceData);
    D3D12_RESOURCE_DESC instRes = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, instBufSize, 1, 1, 1,
        DXGI_FORMAT_UNKNOWN, {1,0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
    device->CreateCommittedResource(&upload, D3D12_HEAP_FLAG_NONE, &instRes,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m.instanceBuffer));
    m.instanceView.BufferLocation = m.instanceBuffer->GetGPUVirtualAddress();
    m.instanceView.SizeInBytes    = instBufSize;
    m.instanceView.StrideInBytes  = sizeof(InstanceData);
}

void Item::UnloadMesh(ItemMesh& m)
{
    m.vb.Reset();  m.vbUpload.Reset();
    m.ib.Reset();  m.ibUpload.Reset();
    m.instanceBuffer.Reset();
    m.indexCount = m.maxInstances = 0;
    m.vbDirty = m.ibDirty = false;
}
