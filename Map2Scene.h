#pragma once
#include "Scene.h"
#include "GameObject.h"
#include "Cube.h"
#include "Player.h"
#include "Gun.h"
#include "Enemy.h"
#include "Bullet.h"
#include "Item.h"
#include "Item_ATK.h"
#include "Item_HP.h"
#include "Item_SPEED.h"

class Map2Scene : public Scene
{
public:
	Map2Scene();
	virtual ~Map2Scene();

	virtual void Initialize() override;
	virtual void Update(float dt) override;
	virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList) override;
	virtual void Release() override;

private:
	std::vector<GameObject*> mGameObjects;
	std::vector<Cube*>       mWallCubes;
	std::vector<Enemy*>      mEnemies;
	Player* mPlayer;
	Gun*    mGun;

	std::vector<DirectX::BoundingBox> mCubeAABBs;

	std::vector<Bullet*> mBullets;
	static constexpr int kBulletPoolSize = 10;
	void FireBullet();
	void CheckBulletEnemyCollision();
	void CheckEnemyPlayerCollision();

	std::vector<Item*> mItems;
	void SpawnItems();
	void CheckItemPickup();

	// Map2는 41x41 그리드 (Map1의 51x51과 다른 구조)
	static constexpr int   kMazeSize    = 41;
	static constexpr float kMazeSpacing = 2.0f;
	static constexpr float kMazeOffset  = (kMazeSize - 1) * kMazeSpacing * 0.5f;
	int mFlowField[kMazeSize][kMazeSize];
	void UpdateFlowField();
};
