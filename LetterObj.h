#pragma once
#include "GameObject.h"
#include <DirectXCollision.h>

class LetterObj : public GameObject
{
public:
	LetterObj();
	virtual ~LetterObj();

	void Initialize(ComPtr<ID3D12Device> device, const std::string& text);
	virtual void Update(float dt) override;

	virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj) override;

	DirectX::BoundingBox GetWorldAABB() const;

private:
	DirectX::BoundingBox mLocalAABB;
	std::array<float, 4> color;
};