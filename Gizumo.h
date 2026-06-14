#pragma once
#include "GameObject.h"

class Gizumo : public GameObject
{
public:
	Gizumo();
	virtual void Initialize(ComPtr<ID3D12Device> device) override;
	virtual void Update(float dt) override;

	virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj) override;

private:
	
};
