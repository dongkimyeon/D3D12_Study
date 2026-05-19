#pragma once
#include "GameObject.h"

class Title : public GameObject
{
public:
	Title();
	virtual ~Title();

	virtual void Initialize(ComPtr<ID3D12Device> device) override;
	virtual void Update(float dt) override;

	// Render 함수 오버라이딩 추가
	virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj) override;

private:
	
	std::array<float, 4> color;
};