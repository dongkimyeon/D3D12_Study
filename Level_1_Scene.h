#pragma once
#include "Scene.h"
#include "GameObject.h"
#include "Helicopter.h"
#include "Map.h"
#include "Missile.h"

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
    void RebuildMapInstances();
    void ApplyFrustumCulling(const DirectX::BoundingFrustum& worldFrustum);

    std::vector<GameObject*> mGameObjects;
	std::unique_ptr<Helicopter> mHelicopter;

    Map* mMap = nullptr;
    float mMapSpacingX = 69.0f * 2.0f;
    float mMapSpacingZ = 89.0f * 2.0f;
    std::vector<XMFLOAT4X4> mAllTileMatrices;
    int mVisibleTileCount = 0;

    static constexpr int kMissilePoolSize = 10;
    Missile* mMissilePool[kMissilePoolSize] = {};
    bool mFireFromLeft = true;
};