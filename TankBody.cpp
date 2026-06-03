#include "TankBody.h"
#include "framework.h"

DirectX::BoundingBox TankBody::sSharedLocalAABB = {};

TankBody::TankBody() {}
TankBody::~TankBody() {}

void TankBody::Initialize(ComPtr<ID3D12Device> device, UINT count)
{
    LoadFromOBJ("Tank_Body.OBJ", device);
    SetColor(1.f, 0.6f, 0.6f);
    sSharedLocalAABB = mLocalAABB;
    CreateInstanceBuffer(device, count);

    // OBB 디버그 드로우용 Upload Heap 버퍼 (count 슬롯)
    D3D12_HEAP_PROPERTIES upload = { D3D12_HEAP_TYPE_UPLOAD };
    UINT obbBufSize = count * sizeof(XMFLOAT4X4);
    D3D12_RESOURCE_DESC obbDesc = {
        D3D12_RESOURCE_DIMENSION_BUFFER, 0, obbBufSize,
        1,1,1,DXGI_FORMAT_UNKNOWN,{1,0},D3D12_TEXTURE_LAYOUT_ROW_MAJOR,D3D12_RESOURCE_FLAG_NONE
    };
    device->CreateCommittedResource(&upload, D3D12_HEAP_FLAG_NONE, &obbDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mOBBInstBuf));
    mOBBInstView.BufferLocation = mOBBInstBuf->GetGPUVirtualAddress();
    mOBBInstView.StrideInBytes  = sizeof(XMFLOAT4X4);
    mOBBInstView.SizeInBytes    = obbBufSize;
}

void TankBody::UpdateInstances(const std::vector<XMFLOAT4X4>& matrices)
{
    if (matrices.empty() || !mInstanceBufferUpload) return;

    mCurrentBodyMats = matrices;
    mInstanceCount   = (UINT)matrices.size();

    void* ptr;
    mInstanceBufferUpload->Map(0, nullptr, &ptr);
    memcpy(ptr, matrices.data(), matrices.size() * sizeof(XMFLOAT4X4));
    mInstanceBufferUpload->Unmap(0, nullptr);
    mInstDirty = true;
}

void TankBody::RenderOBBs(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
    if (!sShowAABB || mCurrentBodyMats.empty()) return;
    EnsureAABBMesh();

    // 모든 인스턴스의 OBB 월드 행렬을 한 번에 계산
    std::vector<XMFLOAT4X4> obbMats;
    obbMats.reserve(mCurrentBodyMats.size());
    for (const auto& mat : mCurrentBodyMats)
    {
        DirectX::BoundingOrientedBox localOBB;
        DirectX::BoundingOrientedBox::CreateFromBoundingBox(localOBB, sSharedLocalAABB);
        DirectX::BoundingOrientedBox worldOBB;
        localOBB.Transform(worldOBB, XMLoadFloat4x4(&mat));

        XMMATRIX world =
            XMMatrixScaling(worldOBB.Extents.x * 2.f, worldOBB.Extents.y * 2.f, worldOBB.Extents.z * 2.f)
            * XMMatrixRotationQuaternion(XMLoadFloat4(&worldOBB.Orientation))
            * XMMatrixTranslation(worldOBB.Center.x, worldOBB.Center.y, worldOBB.Center.z);

        XMFLOAT4X4 worldData;
        XMStoreFloat4x4(&worldData, world);
        obbMats.push_back(worldData);
    }

    void* ptr;
    mOBBInstBuf->Map(0, nullptr, &ptr);
    memcpy(ptr, obbMats.data(), obbMats.size() * sizeof(XMFLOAT4X4));
    mOBBInstBuf->Unmap(0, nullptr);

    XMFLOAT4X4 vpFloat;
    XMStoreFloat4x4(&vpFloat, XMMatrixTranspose(view * proj));

    commandList->SetGraphicsRoot32BitConstants(0, 16, &vpFloat.m[0][0], 0);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    commandList->IASetVertexBuffers(0, 1, &sAABBVbView);
    commandList->IASetVertexBuffers(1, 1, &mOBBInstView);
    commandList->IASetIndexBuffer(&sAABBIbView);
    commandList->DrawIndexedInstanced(24, (UINT)obbMats.size(), 0, 0, 0);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void TankBody::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
    if (mInstanceCount == 0) return;
    GameObject::Render(commandList, view, proj);
}
