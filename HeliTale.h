#pragma once
#include "GameObject.h"

class HeliTale : public GameObject
{
public:
	HeliTale();
	~HeliTale();

	void Initialize(ComPtr<ID3D12Device> device) override;
	void Update(float dt) override;
	void Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj) override;

private:
	float rotationSpeed;
};

