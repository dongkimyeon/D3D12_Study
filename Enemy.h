#pragma once
#include "GameObject.h"
#include <DirectXCollision.h>

class Enemy : public GameObject
{
public:
	Enemy();
	virtual ~Enemy();

	virtual void Initialize(ComPtr<ID3D12Device> device) override;
	virtual void Update(float dt) override;
	virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj) override;
	virtual DirectX::BoundingBox GetWorldAABB() const override;

	// 씬 Initialize/Release에서 명시적으로 호출
	static void LoadSharedMesh(ComPtr<ID3D12Device> device);
	static void UnloadSharedMesh();

private:
	DirectX::BoundingBox mLocalAABB;

	static ComPtr<ID3D12Resource>   sVB;
	static ComPtr<ID3D12Resource>   sIB;
	static ComPtr<ID3D12Resource>   sVBUpload;
	static ComPtr<ID3D12Resource>   sIBUpload;
	static D3D12_VERTEX_BUFFER_VIEW sVbView;
	static D3D12_INDEX_BUFFER_VIEW  sIbView;
	static UINT                     sIndexCount;
	static D3D12_RESOURCE_STATES    sVBState;
	static D3D12_RESOURCE_STATES    sIBState;
	static bool                     sVBDirty;
	static bool                     sIBDirty;
	static DirectX::BoundingBox     sLocalAABB;
};
