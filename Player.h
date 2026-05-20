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
	float    GetPitch()      const { return mPitch; }
	XMFLOAT3 GetForwardDir() const { return { sinf(mYaw), 0.0f, cosf(mYaw) }; }
	XMFLOAT3 GetLookDir()    const {
		return { cosf(mPitch) * sinf(mYaw), -sinf(mPitch), cosf(mPitch) * cosf(mYaw) };
	}
	void SetColliders(const std::vector<DirectX::BoundingBox>* colliders) { mColliders = colliders; }

	// 스탯
	int   GetHP()        const { return mHP; }
	int   GetATK()       const { return mATK; }
	float GetMoveSpeed() const { return mMoveSpeed; }

	void AddHP(int amount)         { mHP = std::min(mHP + amount, kMaxHP); }
	void AddATK(int amount)        { mATK += amount; }
	void AddMoveSpeed(float amount){ mMoveSpeed += amount; }

private:
	// 현재 position + mYaw 기준으로 물리 AABB 계산 (worldMatrix 갱신 전에도 사용 가능)
	DirectX::BoundingBox ComputePhysicsAABB() const;

	void ResolveHorizontal(const std::vector<DirectX::BoundingBox>& cubes);
	void ResolveVertical  (const std::vector<DirectX::BoundingBox>& cubes);

	DirectX::BoundingBox mLocalAABB;

	// 스탯
	int   mHP        = 100;
	int   mATK       = 20;
	static constexpr int kMaxHP = 100;

	// 이동/시점
	float mPitch      = 0.0f;
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
