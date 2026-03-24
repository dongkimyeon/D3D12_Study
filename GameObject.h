#pragma once
#include "stdafx.h" // OBJVertex, DirectXMath 등 포함

class GameObject
{
public:
    GameObject();
    virtual ~GameObject();

    virtual void Initialize(ComPtr<ID3D12Device> device);
    virtual void Update(float dt);

    virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj);

    void LoadFromOBJ(const std::string& filename, ComPtr<ID3D12Device> device);
    void SetPosition(float x, float y, float z);
	void SetRotation(float pitch, float yaw, float roll);

	void BuildNormalBuffer(ComPtr<ID3D12Device> device);


protected:
    // 정점 데이터를 VRAM 버퍼에 새로 업데이트하는 함수
    void UpdateVertexBuffer();

public:
    XMFLOAT3 position;
	XMFLOAT3 rotation;

protected:
    std::vector<OBJVertex> vertices;
    std::vector<uint16_t> indices;
    XMMATRIX worldMatrix;

    ComPtr<ID3D12Resource> vertexBuffer;
    ComPtr<ID3D12Resource> indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vbView;
    D3D12_INDEX_BUFFER_VIEW ibView;



	// 노멀 라인 렌더링을 위한 멤버 변수들
	ComPtr<ID3D12Resource> normalVertexBuffer;
	ComPtr<ID3D12Resource> normalIndexBuffer;
	D3D12_VERTEX_BUFFER_VIEW normalVbView = {};
	D3D12_INDEX_BUFFER_VIEW normalIbView = {};
	UINT normalIndexCount = 0;

};