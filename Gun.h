#pragma once
#include "GameObject.h"

class Player;

class Gun : public GameObject
{
public:
	Gun();
	virtual ~Gun();

	virtual void Initialize(ComPtr<ID3D12Device> device) override;
	virtual void Update(float dt) override;
	virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj) override;

	void     AttachTo(Player* player) { mOwner = player; }
	XMFLOAT3 GetMuzzlePosition() const;

private:
	Player* mOwner = nullptr;

	// 플레이어 로컬 공간 기준 오프셋 (우, 상, 전)
	static constexpr float kOffsetRight   =  0.35f;
	static constexpr float kOffsetUp      = -0.20f;
	static constexpr float kOffsetForward =  0.40f;
};