#include "Crosshair.h"

Crosshair::Crosshair() {}

void Crosshair::Initialize(ComPtr<ID3D12Device> device)
{
    GameObject::Initialize(device);

    constexpr float size = 0.025f;  
    constexpr float gap  = 0.005f; 

    constexpr float ln = 0.5774f;

    vertices = {

        { -size, 0.0f, 0.0f,  ln, ln, -ln,  1, 1, 1, 1 },
        {  -gap, 0.0f, 0.0f,  ln, ln, -ln,  1, 1, 1, 1 },

        {   gap, 0.0f, 0.0f,  ln, ln, -ln,  1, 1, 1, 1 },
        {  size, 0.0f, 0.0f,  ln, ln, -ln,  1, 1, 1, 1 },

        { 0.0f,  size, 0.0f,  ln, ln, -ln,  1, 1, 1, 1 },
        { 0.0f,   gap, 0.0f,  ln, ln, -ln,  1, 1, 1, 1 },
  
        { 0.0f,  -gap, 0.0f,  ln, ln, -ln,  1, 1, 1, 1 },
        { 0.0f, -size, 0.0f,  ln, ln, -ln,  1, 1, 1, 1 },
    };

    indices = {
        0, 1,
        2, 3,   
        4, 5,
        6, 7,  
    };

    CreateBuffersFromData(device);
}

void Crosshair::Update(float dt) {}

void Crosshair::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX , XMMATRIX )
{
    // 크로스헤어는 스크린-스페이스 → viewProj = identity
    XMFLOAT4X4 identityT;
    XMStoreFloat4x4(&identityT, XMMatrixTranspose(XMMatrixIdentity()));
    commandList->SetGraphicsRoot32BitConstants(0, 16, &identityT.m[0][0], 0);

    // 슬롯 1 인스턴스 버퍼에 world=identity, color=white 기록
    InstanceData inst;
    XMStoreFloat4x4(&inst.world, XMMatrixTranspose(XMMatrixIdentity()));
    inst.color = { 1.f, 1.f, 1.f, 1.f };
    void* mapped = nullptr;
    mInstanceBuffer->Map(0, nullptr, &mapped);
    memcpy(mapped, &inst, sizeof(InstanceData));
    mInstanceBuffer->Unmap(0, nullptr);

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

    UploadBufferIfDirty(commandList, vertexBuffer, vertexBufferUpload,
        mVBState, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
        vertices.size() * sizeof(OBJVertex), mVBDirty);
    UploadBufferIfDirty(commandList, indexBuffer, indexBufferUpload,
        mIBState, D3D12_RESOURCE_STATE_INDEX_BUFFER,
        indices.size() * sizeof(uint16_t), mIBDirty);

    commandList->IASetVertexBuffers(0, 1, &vbView);
    commandList->IASetVertexBuffers(1, 1, &mInstanceBufView);
    commandList->IASetIndexBuffer(&ibView);
    commandList->DrawIndexedInstanced(static_cast<UINT>(indices.size()), 1, 0, 0, 0);

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}
