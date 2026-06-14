#pragma once
#include "Scene.h"
#include "GameObject.h"
#include "Tank.h"
#include "TankBody.h"
#include "TankLid.h"
#include "TankBarrel.h"
#include "Terrain.h"
#include "Missile.h"
#include "ExplosionSystem.h"
#include "Cube.h"
#include "LetterObj.h"
class Level_2_Scene : public Scene
{
public:
    Level_2_Scene() = default;
    virtual ~Level_2_Scene() = default;

    virtual void Initialize() override;
    virtual void Update(float dt) override;
    virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList) override;
    virtual void Release() override;
private:
	std::vector<GameObject*> mGameObjects;

	std::unique_ptr<TankBody>   mTankBodyRenderer;
	std::unique_ptr<TankLid>    mTankLidRenderer;
	std::unique_ptr<TankBarrel> mTankBarrelRenderer;
	std::vector<std::unique_ptr<Tank>> mTanks;

	std::unique_ptr<TankBody>   mPlayerBodyRenderer;
	std::unique_ptr<TankLid>    mPlayerLidRenderer;
	std::unique_ptr<TankBarrel> mPlayerBarrelRenderer;
	std::unique_ptr<Tank>       mPlayerTank;

	std::unique_ptr<Terrain>    mTerrain;

	std::vector<XMFLOAT4X4> mBodyMats;
	std::vector<XMFLOAT4X4> mLidMats;
	std::vector<XMFLOAT4X4> mBarrelMats;

	static constexpr int kMissilePoolSize = 8;
	Missile* mMissilePool[kMissilePoolSize] = {};

	int   mSelectedTank = -1;

	bool  mMouseRotating = false;
	POINT mPrevMousePos  = {};

	std::unique_ptr<ExplosionSystem> mExplosion;

	std::unique_ptr<GameObject> mShield;
	bool mShieldActive = false;

	std::unique_ptr<LetterObj> mYouWin;
	bool  mWinActive  = false;
	float mWinTimer   = 0.f;
};
