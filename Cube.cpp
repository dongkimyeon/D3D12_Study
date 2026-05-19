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

void Cube::LoadSharedMesh(ComPtr<ID3D12Device> device)
{
    std::vector<OBJVertex> verts;
    std::vector<uint16_t>  inds;
    OBJLoader::Load("cube.obj", verts, inds);

    for (auto& v : verts) { v.r = v.g = v.b = v.a = 1.0f; }

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
    sIndexCount = 0;
    sVBDirty = sIBDirty = false;
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
    mAABB.Center  = position;
    mAABB.Extents = { scale.x * 0.5f, scale.y * 0.5f, scale.z * 0.5f };
}

void Cube::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
    if (sIndexCount == 0) return;

    // Upload shared VB/IB to GPU on the first Render call (dirty flag → false after)
    auto flushIfDirty = [&](ComPtr<ID3D12Resource>& gpu, ComPtr<ID3D12Resource>& staging,
        D3D12_RESOURCE_STATES& state, D3D12_RESOURCE_STATES target,
        UINT64 byteSize, bool& dirty)
    {
        if (!dirty) return;
        if (state != D3D12_RESOURCE_STATE_COPY_DEST)
        {
            D3D12_RESOURCE_BARRIER b = {};
            b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            b.Transition.pResource   = gpu.Get();
            b.Transition.StateBefore = state;
            b.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
            b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            commandList->ResourceBarrier(1, &b);
        }
        commandList->CopyBufferRegion(gpu.Get(), 0, staging.Get(), 0, byteSize);
        {
            D3D12_RESOURCE_BARRIER b = {};
            b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            b.Transition.pResource   = gpu.Get();
            b.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            b.Transition.StateAfter  = target;
            b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            commandList->ResourceBarrier(1, &b);
        }
        state = target;
        dirty = false;
    };

    flushIfDirty(sVB, sVBUpload, sVBState,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
        sVbView.SizeInBytes, sVBDirty);
    flushIfDirty(sIB, sIBUpload, sIBState,
        D3D12_RESOURCE_STATE_INDEX_BUFFER,
        sIbView.SizeInBytes, sIBDirty);

    // Root constants layout (20 floats total):
    //   [0..3]  = mColor (per-instance tint, read as dummyColor in shader)
    //   [4..19] = MVP matrix (transposed)
    XMMATRIX world = XMLoadFloat4x4(&worldMatrix);
    XMMATRIX mvp   = world * view * proj;
    XMFLOAT4X4 mvpT;
    XMStoreFloat4x4(&mvpT, XMMatrixTranspose(mvp));

    commandList->SetGraphicsRoot32BitConstants(0, 4,  &mColor,       0);
    commandList->SetGraphicsRoot32BitConstants(0, 16, &mvpT.m[0][0], 4);

    commandList->IASetVertexBuffers(0, 1, &sVbView);
    commandList->IASetIndexBuffer(&sIbView);
    commandList->DrawIndexedInstanced(sIndexCount, 1, 0, 0, 0);

    if (sShowAABB)
        RenderAABB(commandList, view, proj);
}

DirectX::BoundingBox Cube::GetWorldAABB() const
{
    return mAABB; // UpdateAABB()에서 매 프레임 월드 공간으로 갱신됨
}
