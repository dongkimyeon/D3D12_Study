#pragma once
#include "GameObject.h"
#include <DirectXCollision.h>

class LetterObj : public GameObject
{
public:
	LetterObj();
	virtual ~LetterObj();

	virtual void Initialize(ComPtr<ID3D12Device> device) override;
	virtual void Update(float dt) override;

	virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj) override;

	// 월드 공간 AABB 반환 (레이 피킹용)
	DirectX::BoundingBox GetWorldAABB() const;

private:
	DirectX::BoundingBox mLocalAABB;
	std::array<float, 4> color;
};