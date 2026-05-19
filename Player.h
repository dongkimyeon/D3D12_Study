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
	virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj) override;
	virtual DirectX::BoundingBox GetWorldAABB() const override;

	float    GetYaw()        const { return mYaw; }
	XMFLOAT3 GetForwardDir() const { return { sinf(mYaw), 0.0f, cosf(mYaw) }; }
	void  SetColliders(const std::vector<DirectX::BoundingBox>* colliders) { mColliders = colliders; }

private:
	// 현재 position + mYaw 기준으로 물리 AABB 계산 (worldMatrix 갱신 전에도 사용 가능)
	DirectX::BoundingBox ComputePhysicsAABB() const;

	void ResolveHorizontal(const std::vector<DirectX::BoundingBox>& cubes);
	void ResolveVertical  (const std::vector<DirectX::BoundingBox>& cubes);

	DirectX::BoundingBox mLocalAABB;

	// 이동/시점
	float mYaw        = 0.0f;
	float mMoveSpeed  = 5.0f;
	bool  mFirstMouse = true;

	// 물리
	float mVelocityY = 0.0f;
	bool  mOnGround  = false;
	const std::vector<DirectX::BoundingBox>* mColliders = nullptr;

	static constexpr float kGravity   = -20.0f;
	static constexpr float kJumpSpeed =   8.0f;
};
