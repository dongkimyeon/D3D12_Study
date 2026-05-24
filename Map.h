#pragma once
#include "GameObject.h"

class Map : public GameObject
{
public:
	Map();
	virtual ~Map();

	virtual void Initialize(ComPtr<ID3D12Device> device) override;
	virtual void Update(float dt) override;

	void AddInstance(XMFLOAT3 position, float yaw = 0.0f);
	void ClearInstances();
	void BuildInstanceBuffer(ComPtr<ID3D12Device> device);
	void UpdateInstancesForCulling(const std::vector<XMFLOAT4X4>& visible);
	DirectX::BoundingBox GetWorldAABB() const override { return DirectX::BoundingBox({ 0,0,0 }, { 0,0,0 }); }

private:
	std::vector<XMFLOAT4X4> mInstances;
};
