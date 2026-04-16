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

	/*-------------------------GETTER--------------------------*/
	XMFLOAT4X4 GetWorldMatrix() const { return worldMatrix; }
	XMFLOAT3 GetPosition() const { return position; }
	XMFLOAT3 GetRotation() const { return rotation; }
	XMFLOAT3 GetScale() const { return scale; }
	XMFLOAT4 GetForwardVector() const { return forward_vector; }

	/*-------------------------SETTER--------------------------*/
	void SetWorldMatrix(const  XMFLOAT4X4 matrix) { worldMatrix = matrix; }

	void SetPosition(float x, float y, float z) { position = { x, y, z }; }
	void SetPosition(XMFLOAT3 pos) { position = pos; }

	void SetRotation(float pitch, float yaw, float roll) { rotation = { pitch, yaw, roll }; }
	void SetRotation(XMFLOAT3 rot) { rotation = rot; }

	void SetScale(float scaleX, float scaleY, float scaleZ) { scale = { scaleX, scaleY, scaleZ }; }
	void SetScale(XMFLOAT3 s) { scale = s; }

	void BakeRotationX(float angleDeg);
	void BuildNormalBuffer(ComPtr<ID3D12Device> device);


protected:
    // 정점 데이터를 VRAM 버퍼에 새로 업데이트하는 함수
    void UpdateVertexBuffer();

protected:
    XMFLOAT3 position;
	XMFLOAT3 rotation;
	XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };
	XMFLOAT4 forward_vector = { 0, 0, 1, 0 }; // 초기 전방 벡터 (Z축 방향)
	XMFLOAT4X4 worldMatrix;
protected:
    std::vector<OBJVertex> vertices;
    std::vector<uint16_t> indices;

    

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