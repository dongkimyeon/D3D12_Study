#pragma once
#include "GameObject.h"
#include <DirectXCollision.h>

class HeliBody : public GameObject
{
public:
	HeliBody();
	virtual ~HeliBody();

	virtual void Initialize(ComPtr<ID3D12Device> device) override;
	virtual void Update(float dt) override;
	virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj) override;

	virtual DirectX::BoundingOrientedBox GetWorldOBB() const override;
	virtual bool UseOBB() const override { return true; }

private:

};

