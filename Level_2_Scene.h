#pragma once
#include "Scene.h"
#include "GameObject.h"
#include "Tank.h"
#include "TankBody.h"
#include "TankLid.h"
#include "TankBarrel.h"
#include "Terrain.h"
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

	bool  mMouseRotating = false;
	POINT mPrevMousePos  = {};
};
