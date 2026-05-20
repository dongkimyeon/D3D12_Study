#pragma once
#include "GameObject.h"
#include <DirectXCollision.h>

class Bullet : public GameObject
{
public:
	Bullet();
	virtual ~Bullet();

	virtual void Initialize(ComPtr<ID3D12Device> device) override;
	virtual void Update(float dt) override;
	virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj) override;
	virtual DirectX::BoundingBox GetWorldAABB() const override;

	void Fire(XMFLOAT3 spawnPos, XMFLOAT3 dir, int damage = 20, float speed = 30.0f);
	bool IsActive()   const { return mActive; }
	int  GetDamage()  const { return mDamage; }
	void Deactivate()       { mActive = false; }
	void SetColliders(const std::vector<DirectX::BoundingBox>* c) { mColliders = c; }

private:
	DirectX::BoundingBox mLocalAABB;

	XMFLOAT3 mDir    = {};
	float    mSpeed  = 30.0f;
	float    mLife   = 0.0f;
	bool     mActive = false;
	int      mDamage = 20;

	const std::vector<DirectX::BoundingBox>* mColliders = nullptr;
};
