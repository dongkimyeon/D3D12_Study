#pragma once
#include "GameObject.h"
#include <DirectXCollision.h>

class Player : public GameObject
{
public:
	Player();
	virtual ~Player();

	virtual void Initialize(ComPtr<ID3D12Device> device) override;
	virtual void Update(float dt) override;

	// Render 함수 오버라이딩 추가
	virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj) override;
	virtual DirectX::BoundingBox GetWorldAABB() const override;

private:
	DirectX::BoundingBox mLocalAABB;

	float mYaw       = 0.0f;
	float mMoveSpeed = 5.0f;
	bool  mFirstMouse = true;

public:
	float GetYaw() const { return mYaw; }
};