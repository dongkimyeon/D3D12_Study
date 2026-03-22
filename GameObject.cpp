#include "stdafx.h"
#include "GameObject.h"

GameObject::GameObject()
{
    position = { 0, 0, 0 };
    color = { 1.0f, 1.0f, 1.0f, 1.0f };
    worldMatrix = XMMatrixIdentity();
}

GameObject::~GameObject()
{
}

void GameObject::Initialize(ComPtr<ID3D12Device> device)
{
    // 추가적인 초기화가 필요하면 작성
}

void GameObject::Update(float dt)
{
    // 개별 오브젝트의 로직 (예: 애니메이션, 회전 등)
}

void GameObject::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
    if (indices.empty()) return; // 로드된 메쉬가 없다면 그리지 않음

    XMMATRIX mvp = worldMatrix * view * proj;
    XMFLOAT4X4 mvpFloat;
    XMStoreFloat4x4(&mvpFloat, XMMatrixTranspose(mvp));

    // 색상과 변환행렬을 루트 상수로 전달 (루트 파라미터가 0 인덱스 배열 하나로 구성되어 있다고 가정)
    commandList->SetGraphicsRoot32BitConstants(0, 4, color.data(), 0);
    commandList->SetGraphicsRoot32BitConstants(0, 16, &mvpFloat.m[0][0], 4);

    commandList->IASetVertexBuffers(0, 1, &vbView);
    commandList->IASetIndexBuffer(&ibView);
    commandList->DrawIndexedInstanced(static_cast<UINT>(indices.size()), 1, 0, 0, 0);
}

void GameObject::LoadFromOBJ(const std::string& filename, ComPtr<ID3D12Device> device)
{
    OBJLoader::Load(filename, vertices, indices);

    // ---------------------------------
    // 정점 버퍼 생성
    UINT maxVertexBufferSize = static_cast<UINT>(vertices.size() * sizeof(OBJVertex));
    D3D12_HEAP_PROPERTIES heap = { D3D12_HEAP_TYPE_UPLOAD };
    D3D12_RESOURCE_DESC vRes = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, maxVertexBufferSize, 1, 1, 1,
                                DXGI_FORMAT_UNKNOWN, {1, 0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
    device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &vRes, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBuffer));

    void* vData;
    vertexBuffer->Map(0, nullptr, &vData);
    memcpy(vData, vertices.data(), vertices.size() * sizeof(OBJVertex));
    vertexBuffer->Unmap(0, nullptr);

    vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vbView.StrideInBytes = sizeof(OBJVertex);
    vbView.SizeInBytes = maxVertexBufferSize;

    // ---------------------------------
    // 인덱스 버퍼 생성
    UINT maxIndexBufferSize = static_cast<UINT>(indices.size() * sizeof(uint16_t));
    D3D12_RESOURCE_DESC iRes = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, maxIndexBufferSize, 1, 1, 1,
                                DXGI_FORMAT_UNKNOWN, {1, 0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
    device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &iRes, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&indexBuffer));

    void* iData;
    indexBuffer->Map(0, nullptr, &iData);
    memcpy(iData, indices.data(), indices.size() * sizeof(uint16_t));
    indexBuffer->Unmap(0, nullptr);

    ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
    ibView.Format = DXGI_FORMAT_R16_UINT;
    ibView.SizeInBytes = maxIndexBufferSize;
}

void GameObject::SetPosition(float x, float y, float z)
{
    position = { x, y, z };
    worldMatrix = XMMatrixTranslation(x, y, z);
}