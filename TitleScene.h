#pragma once
#include "Scene.h"
#include "GameObject.h"
#include "LetterObj.h"
#include "Cube.h"

class TitleScene : public Scene
{
public:
	TitleScene();
	virtual ~TitleScene();

	virtual void Initialize() override;
	virtual void Update(float dt) override;
	virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList) override;
	virtual void Release() override;

private:
	void SpawnExplosion();

	struct ExplodeCube
	{
		Cube*     cube;
		XMFLOAT3  velocity;
		XMFLOAT3  angularVel;
	};

	std::vector<GameObject*>  mGameObjects;
	LetterObj* mTitleText1 = nullptr;
	LetterObj* mTitleText2 = nullptr;
	LetterObj* mName       = nullptr;

	std::vector<ExplodeCube> mExplosionCubes;
	bool  mExploding     = false;
	float mExplodeTimer  = 0.f;
	static constexpr float kExplodeDuration = 0.8f;
};

