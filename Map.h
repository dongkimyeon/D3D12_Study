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
	void BuildInstanceBuffer(ComPtr<ID3D12Device> device);

private:
	std::vector<XMFLOAT4X4> mInstances;
};
