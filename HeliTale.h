#pragma once
#include "GameObject.h"

class HeliTale : public GameObject
{
public:
	HeliTale();
	~HeliTale();

	void Initialize(ComPtr<ID3D12Device> device) override;
	DirectX::BoundingBox GetWorldAABB() const override { return DirectX::BoundingBox({ 0,0,0 }, { 0,0,0 }); }

private:
	float rotationSpeed;
};

