#pragma once
#include "Scene.h"
#include "GameObject.h"
#include "Helicopter.h"
#include "Tank.h"
#include "TankBody.h"
#include "TankLid.h"
#include "TankBarrel.h"
#include "Ring.h"
#include "Plane.h"
#include "Missile.h"
#include "Terrain.h"

class Level_1_Scene : public Scene
{
public:
    Level_1_Scene();
    virtual ~Level_1_Scene();

    virtual void Initialize() override;
    virtual void Update(float dt) override;
    virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList) override;
    virtual void Release() override;

private:
    std::vector<GameObject*> mGameObjects;
    std::unique_ptr<Helicopter>        mHelicopter;

	std::unique_ptr<Terrain>    mTerrain;	
    Ring*    mRing    = nullptr;
    Plane*   mPlane   = nullptr;

    static constexpr int kMissilePoolSize = 10;
    Missile* mMissilePool[kMissilePoolSize] = {};
    bool mFireFromLeft = true;

    bool     mFirstPerson  = false;
    XMFLOAT3 mFpvOffset    = { 0.f, -0.780f, 5.35f };
    int      mSelectedTank = 0;
};
