#pragma once
#include "stdafx.h"

class GameObject
{
public:
	GameObject();
	virtual ~GameObject();

	virtual void Initialize(ComPtr<ID3D12Device> device);
	virtual void Update(float dt);

	virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj);

	void LoadFromOBJ(const std::string& filename, ComPtr<ID3D12Device> device);
	void CreateInstanceBuffer(ComPtr<ID3D12Device> device, UINT count);

	XMFLOAT4X4 GetWorldMatrix() const { return worldMatrix; }
	XMFLOAT3 GetPosition() const { return position; }
	const XMFLOAT3* GetPositionPtr() const { return &position; }
	XMFLOAT3 GetRotation() const { return rotation; }
	XMFLOAT3 GetScale() const { return scale; }
	XMFLOAT4 GetForwardVector() const { return forward_vector; }
	size_t GetVertexCount() const { return vertices.size(); }
	size_t GetIndexCount()  const { return indices.size(); }
	const OBJVertex* GetFirstVertex() const { return vertices.empty() ? nullptr : &vertices[0]; }

	void SetWorldMatrix(const  XMFLOAT4X4 matrix) { worldMatrix = matrix; }

	void SetPosition(float x, float y, float z) { position = { x, y, z }; }
	void SetPosition(XMFLOAT3 pos) { position = pos; }

	void SetRotation(float pitch, float yaw, float roll) { rotation = { XMConvertToRadians(pitch), XMConvertToRadians(yaw), XMConvertToRadians(roll) }; }
	void SetRotation(XMFLOAT3 rot) { rotation = rot; }

	void SetScale(float scaleX, float scaleY, float scaleZ) { scale = { scaleX, scaleY, scaleZ }; }
	void SetScale(XMFLOAT3 s) { scale = s; }

	void BakeScale(float sx, float sy, float sz);
	void BakeRotation(float pitch, float yaw, float roll);
	void BakeRotationX(float angleDeg);
	void RecomputeLocalAABB();
	void SetAlpha(float alpha);
	void SetColor(float r, float g, float b);

	static bool sShowAABB;
	virtual DirectX::BoundingBox        GetWorldAABB() const;
	virtual DirectX::BoundingOrientedBox GetWorldOBB()  const;
	virtual bool UseOBB() const { return false; }
	void RenderAABB(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj);

protected:

	void UpdateVertexBuffer();

	void CreateBuffersFromData(ComPtr<ID3D12Device> device);

	void UploadBufferIfDirty(
		ComPtr<ID3D12GraphicsCommandList>& cmdList,
		ComPtr<ID3D12Resource>& gpuBuf,
		ComPtr<ID3D12Resource>& uploadBuf,
		D3D12_RESOURCE_STATES& currentState,
		D3D12_RESOURCE_STATES targetState,
		UINT64 byteSize,
		bool& dirty);

protected:
	XMFLOAT3 position;
	XMFLOAT3 rotation;
	XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };
	DirectX::BoundingBox mLocalAABB;
	XMFLOAT4 forward_vector = { 0, 0, 1, 0 };
	XMFLOAT4X4 worldMatrix;
protected:
	std::vector<OBJVertex> vertices;
	std::vector<uint32_t> indices;

	ComPtr<ID3D12Resource> vertexBuffer;
	ComPtr<ID3D12Resource> indexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vbView;
	D3D12_INDEX_BUFFER_VIEW ibView;

	ComPtr<ID3D12Resource> vertexBufferUpload;
	ComPtr<ID3D12Resource> indexBufferUpload;

	D3D12_RESOURCE_STATES mVBState = D3D12_RESOURCE_STATE_COPY_DEST;
	D3D12_RESOURCE_STATES mIBState = D3D12_RESOURCE_STATE_COPY_DEST;

	bool mVBDirty = false;
	bool mIBDirty = false;

	ComPtr<ID3D12Resource>   mInstanceBuffer;
	ComPtr<ID3D12Resource>   mInstanceBufferUpload;
	D3D12_VERTEX_BUFFER_VIEW mInstanceBufView = {};
	D3D12_RESOURCE_STATES    mInstState = D3D12_RESOURCE_STATE_COPY_DEST;
	bool                     mInstDirty = false;
	UINT                     mInstanceCount = 1;

	static void EnsureAABBMesh();
	static ComPtr<ID3D12Resource>   sAABBVB;
	static ComPtr<ID3D12Resource>   sAABBIB;
	static D3D12_VERTEX_BUFFER_VIEW sAABBVbView;
	static D3D12_INDEX_BUFFER_VIEW  sAABBIbView;

	ComPtr<ID3D12Resource>   mAABBInstBuf;
	D3D12_VERTEX_BUFFER_VIEW mAABBInstView = {};
};