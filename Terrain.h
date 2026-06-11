#pragma once
#include "GameObject.h"


class Terrain : public GameObject
{
public:
	Terrain();
	virtual ~Terrain();

	virtual void Initialize(ComPtr<ID3D12Device> device) override;
	virtual void Update(float dt) override;
	virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj) override;

	// 월드 좌표 (wx, wz)에서 지형 높이(월드 Y) 반환
	float GetHeightAt(float wx, float wz) const;

private:
	std::vector<float> mHmData;
	int   mHmW = 0, mHmH = 0;
	float mStartX = 0, mStartZ = 0;
	float mWidth  = 0, mDepth  = 0;
	float mMaxY   = 0;
};

