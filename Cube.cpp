#include "stdafx.h"
#include "Cube.h"

ComPtr<ID3D12Resource> Cube::sVB;
ComPtr<ID3D12Resource> Cube::sIB;
ComPtr<ID3D12Resource> Cube::sVBUpload;
ComPtr<ID3D12Resource> Cube::sIBUpload;
D3D12_VERTEX_BUFFER_VIEW Cube::sVbView = {};
D3D12_INDEX_BUFFER_VIEW  Cube::sIbView = {};
UINT    Cube::sIndexCount = 0;
D3D12_RESOURCE_STATES Cube::sVBState  = D3D12_RESOURCE_STATE_COPY_DEST;
D3D12_RESOURCE_STATES Cube::sIBState  = D3D12_RESOURCE_STATE_COPY_DEST;
bool    Cube::sVBDirty = false;
bool    Cube::sIBDirty = false;
DirectX::BoundingBox Cube::sLocalAABB;
ComPtr<ID3D12Resource>   Cube::sInstanceBuffer;
D3D12_VERTEX_BUFFER_VIEW Cube::sInstanceView = {};
UINT                     Cube::sInstanceCount = 0;

void Cube::LoadSharedMesh(ComPtr<ID3D12Device> device)
{
    std::vector<OBJVertex> verts;
    std::vector<uint16_t>  inds;
    OBJLoader::Load("cube.obj", verts, inds);

    for (auto& v : verts) { v.r = v.g = v.b = v.a = 1.0f; }

    // 실제 버텍스 범위로 로컬 AABB 계산 (scale * 0.5 가정 제거)
    XMFLOAT3 vmin = {  FLT_MAX,  FLT_MAX,  FLT_MAX };
    XMFLOAT3 vmax = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
    for (const auto& v : verts) {
        vmin.x = std::min(vmin.x, v.x); vmax.x = std::max(vmax.x, v.x);
        vmin.y = std::min(vmin.y, v.y); vmax.y = std::max(vmax.y, v.y);
        vmin.z = std::min(vmin.z, v.z); vmax.z = std::max(vmax.z, v.z);
    }
    sLocalAABB = DirectX::BoundingBox(
        { (vmin.x + vmax.x) * 0.5f, (vmin.y + vmax.y) * 0.5f, (vmin.z + vmax.z) * 0.5f },
        { (vmax.x - vmin.x) * 0.5f, (vmax.y - vmin.y) * 0.5f, (vmax.z - vmin.z) * 0.5f });

    D3D12_HEAP_PROPERTIES upload = { D3D12_HEAP_TYPE_UPLOAD };
    D3D12_HEAP_PROPERTIES def    = { D3D12_HEAP_TYPE_DEFAULT };

    // Vertex buffer
    UINT vbSize = (UINT)(verts.size() * sizeof(OBJVertex));
    D3D12_RESOURCE_DESC vRes = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, vbSize,
        1, 1, 1, DXGI_FORMAT_UNKNOWN, {1,0},
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };

    device->CreateCommittedResource(&upload, D3D12_HEAP_FLAG_NONE, &vRes,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&sVBUpload));
    device->CreateCommittedResource(&def, D3D12_HEAP_FLAG_NONE, &vRes,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&sVB));

    void* mapped;
    sVBUpload->Map(0, nullptr, &mapped);
    memcpy(mapped, verts.data(), vbSize);
    sVBUpload->Unmap(0, nullptr);

    sVbView.BufferLocation = sVB->GetGPUVirtualAddress();
    sVbView.StrideInBytes  = sizeof(OBJVertex);
    sVbView.SizeInBytes    = vbSize;
    sVBState = D3D12_RESOURCE_STATE_COPY_DEST;
    sVBDirty = true;

    // Index buffer
    UINT ibSize = (UINT)(inds.size() * sizeof(uint16_t));
    D3D12_RESOURCE_DESC iRes = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, ibSize,
        1, 1, 1, DXGI_FORMAT_UNKNOWN, {1,0},
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };

    device->CreateCommittedResource(&upload, D3D12_HEAP_FLAG_NONE, &iRes,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&sIBUpload));
    device->CreateCommittedResource(&def, D3D12_HEAP_FLAG_NONE, &iRes,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&sIB));

    sIBUpload->Map(0, nullptr, &mapped);
    memcpy(mapped, inds.data(), ibSize);
    sIBUpload->Unmap(0, nullptr);

    sIbView.BufferLocation = sIB->GetGPUVirtualAddress();
    sIbView.Format         = DXGI_FORMAT_R16_UINT;
    sIbView.SizeInBytes    = ibSize;
    sIBState = D3D12_RESOURCE_STATE_COPY_DEST;
    sIBDirty = true;

    sIndexCount = (UINT)inds.size();
}

void Cube::UnloadSharedMesh()
{
    sVB.Reset();  sVBUpload.Reset();
    sIB.Reset();  sIBUpload.Reset();
    sInstanceBuffer.Reset();
    sIndexCount = sInstanceCount = 0;
    sVBDirty = sIBDirty = false;
}

void Cube::BuildInstanceBuffer(ComPtr<ID3D12Device> device, const std::vector<Cube*>& cubes)
{
    sInstanceCount = (UINT)cubes.size();
    if (sInstanceCount == 0) return;

    UINT bufSize = sInstanceCount * sizeof(InstanceData);
    D3D12_HEAP_PROPERTIES upload = { D3D12_HEAP_TYPE_UPLOAD };
    D3D12_RESOURCE_DESC desc = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, bufSize, 1, 1, 1,
        DXGI_FORMAT_UNKNOWN, {1,0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
    device->CreateCommittedResource(&upload, D3D12_HEAP_FLAG_NONE, &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&sInstanceBuffer));

    void* mapped;
    sInstanceBuffer->Map(0, nullptr, &mapped);
    InstanceData* ptr = reinterpret_cast<InstanceData*>(mapped);
    for (UINT i = 0; i < sInstanceCount; ++i)
    {
        ptr[i].world = cubes[i]->worldMatrix;
        ptr[i].color = cubes[i]->mColor;
    }
    sInstanceBuffer->Unmap(0, nullptr);

    sInstanceView.BufferLocation = sInstanceBuffer->GetGPUVirtualAddress();
    sInstanceView.SizeInBytes    = bufSize;
    sInstanceView.StrideInBytes  = sizeof(InstanceData);
}

void Cube::RenderBatch(ComPtr<ID3D12GraphicsCommandList>& commandList)
{
    if (sIndexCount == 0 || sInstanceCount == 0) return;

    // 공유 VB/IB 첫 프레임 GPU 업로드
    auto flush = [&](ComPtr<ID3D12Resource>& gpu, ComPtr<ID3D12Resource>& staging,
        D3D12_RESOURCE_STATES& state, D3D12_RESOURCE_STATES target, UINT64 size, bool& dirty)
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
        commandList->CopyBufferRegion(gpu.Get(), 0, staging.Get(), 0, size);
        D3D12_RESOURCE_BARRIER b = {};
        b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition.pResource   = gpu.Get();
        b.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        b.Transition.StateAfter  = target;
        b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &b);
        state = target; dirty = false;
    };
    flush(sVB, sVBUpload, sVBState, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, sVbView.SizeInBytes, sVBDirty);
    flush(sIB, sIBUpload, sIBState, D3D12_RESOURCE_STATE_INDEX_BUFFER,               sIbView.SizeInBytes, sIBDirty);

    commandList->IASetVertexBuffers(0, 1, &sVbView);
    commandList->IASetVertexBuffers(1, 1, &sInstanceView);
    commandList->IASetIndexBuffer(&sIbView);
    commandList->DrawIndexedInstanced(sIndexCount, sInstanceCount, 0, 0, 0);
}


Cube::Cube()
{
}

Cube::~Cube()
{
}

void Cube::Initialize(ComPtr<ID3D12Device> device)
{
    std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<float> dis(0.3f, 1.0f);
    mColor = { dis(gen), dis(gen), dis(gen), 1.0f };
}

void Cube::Update(float dt)
{
    GameObject::Update(dt);
    UpdateAABB();
}

void Cube::UpdateAABB()
{
    // worldMatrix는 GameObject::Update()에서 이미 갱신됨
    sLocalAABB.Transform(mAABB, XMLoadFloat4x4(&worldMatrix));
}

void Cube::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
    // 메쉬 드로우는 RenderBatch에서 처리 — 여기서는 AABB 디버그만
    if (sShowAABB)
        RenderAABB(commandList, view, proj);
}

DirectX::BoundingBox Cube::GetWorldAABB() const
{
    return mAABB; // UpdateAABB()에서 매 프레임 월드 공간으로 갱신됨
}
