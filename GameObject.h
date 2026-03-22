#pragma once
#include "stdafx.h" // OBJVertex, DirectXMath 등 포함

class GameObject
{
public:
    GameObject();
    virtual ~GameObject();

    virtual void Initialize(ComPtr<ID3D12Device> device);
    virtual void Update(float dt);

    // View, Proj 행렬을 받아오도록 구성
    virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj);

    void LoadFromOBJ(const std::string& filename, ComPtr<ID3D12Device> device);
    void SetPosition(float x, float y, float z);

public:
    XMFLOAT3 position;
    std::array<float, 4> color;

protected:
    std::vector<OBJVertex> vertices;
    std::vector<uint16_t> indices;
    XMMATRIX worldMatrix;

    ComPtr<ID3D12Resource> vertexBuffer;
    ComPtr<ID3D12Resource> indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vbView;
    D3D12_INDEX_BUFFER_VIEW ibView;
};