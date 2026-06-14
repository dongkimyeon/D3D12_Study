#pragma once
#include "Scene.h"
#include "GameObject.h"
#include "Helicopter.h"
#include "Missile.h"
#include "Terrain.h"
#include "Tank.h"
#include "TankBody.h"
#include "TankLid.h"
#include "TankBarrel.h"

class Level_3_Scene : public Scene
{
public:
    Level_3_Scene();
    virtual ~Level_3_Scene();

    virtual void Initialize() override;
    virtual void Update(float dt) override;
    virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList) override;
    virtual void Release() override;

private:
    std::vector<GameObject*> mGameObjects;
    std::unique_ptr<Helicopter> mHelicopter;
    std::unique_ptr<Terrain>    mTerrain;

    static constexpr int kMissilePoolSize = 10;
    Missile* mMissilePool[kMissilePoolSize] = {};
    bool mFireFromLeft = true;

    bool     mFirstPerson = false;
    XMFLOAT3 mFpvOffset   = { 0.f, -0.780f, 5.35f };

    std::unique_ptr<TankBody>   mTankBodyRenderer;
    std::unique_ptr<TankLid>    mTankLidRenderer;
    std::unique_ptr<TankBarrel> mTankBarrelRenderer;
    std::vector<std::unique_ptr<Tank>> mTanks;

    std::vector<XMFLOAT4X4> mBodyMats;
    std::vector<XMFLOAT4X4> mLidMats;
    std::vector<XMFLOAT4X4> mBarrelMats;
};
