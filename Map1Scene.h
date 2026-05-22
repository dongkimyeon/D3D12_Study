#pragma once
#include "Scene.h"
#include "GameObject.h"
#include "Cube.h"
#include "Player.h"
#include "Gun.h"
#include "Enemy.h"
#include "Bullet.h"

class Map1Scene : public Scene
{
public:
	Map1Scene();
	virtual ~Map1Scene();

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

	// 큐브 AABB 캐시 (큐브는 이동하지 않으므로 초기화 시 1회 생성)
	std::vector<DirectX::BoundingBox> mCubeAABBs;

	std::vector<Bullet*> mBullets;
	static constexpr int kBulletPoolSize = 10;
	void FireBullet();
	void CheckBulletEnemyCollision();
	void CheckEnemyPlayerCollision();

	// Flow field (BFS from player, 각 셀 = 플레이어까지 거리)
	static constexpr int   kMazeSize    = 51;
	static constexpr float kMazeSpacing = 2.0f;
	static constexpr float kMazeOffset  = (kMazeSize - 1) * kMazeSpacing * 0.5f;
	int mFlowField[kMazeSize][kMazeSize];
	void UpdateFlowField();
};