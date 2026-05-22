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
    XMFLOAT4X4 identity;
    XMStoreFloat4x4(&identity, XMMatrixTranspose(XMMatrixIdentity()));

    static const float white[4] = { 1.f, 1.f, 1.f, 1.f };
    commandList->SetGraphicsRoot32BitConstants(0, 4,  white,               0);
    commandList->SetGraphicsRoot32BitConstants(0, 16, &identity.m[0][0],   4);

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

    UploadBufferIfDirty(commandList, vertexBuffer, vertexBufferUpload,
        mVBState, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
        vertices.size() * sizeof(OBJVertex), mVBDirty);
    UploadBufferIfDirty(commandList, indexBuffer, indexBufferUpload,
        mIBState, D3D12_RESOURCE_STATE_INDEX_BUFFER,
        indices.size() * sizeof(uint16_t), mIBDirty);

    commandList->IASetVertexBuffers(0, 1, &vbView);
    commandList->IASetIndexBuffer(&ibView);
    commandList->DrawIndexedInstanced(static_cast<UINT>(indices.size()), 1, 0, 0, 0);

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}
