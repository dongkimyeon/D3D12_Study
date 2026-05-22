#include "Map2Scene.h"
#include "Cube.h"
#include "framework.h"
#include <random>
#include <algorithm>
#include "Camera.h"
#include "Player.h"
#include "Bullet.h"
#include "Crosshair.h"
#include "Item_ATK.h"
#include "Item_HP.h"
#include "Item_SPEED.h"

extern bool debugMode;

Map2Scene::Map2Scene() {}
Map2Scene::~Map2Scene() {}

static const int GRID_SIZE2 = 41;
static const float SPACING2 = 2.0f;
static const float OFFSET2 = (GRID_SIZE2 - 1) * SPACING2 * 0.5f;

static int sMaze2[GRID_SIZE2][GRID_SIZE2];

static const int DR2[] = { 0,  0, -2,  2 };
static const int DC2[] = { -2,  2,  0,  0 };

static void Shuffle2(int* arr, int n)
{
	for (int i = n - 1; i > 0; --i)
	{
		int j = rand() % (i + 1);
		int tmp = arr[i];
		arr[i] = arr[j];
		arr[j] = tmp;
	}
}

static void CarvePath2(int r, int c)
{
	sMaze2[r][c] = 1;
	int order[4] = { 0, 1, 2, 3 };
	Shuffle2(order, 4);
	for (int i = 0; i < 4; ++i)
	{
		int d = order[i];
		int nr = r + DR2[d];
		int nc = c + DC2[d];
		if (nr >= 1 && nr < GRID_SIZE2 - 1 &&
			nc >= 1 && nc < GRID_SIZE2 - 1 &&
			sMaze2[nr][nc] == 0)
		{
			sMaze2[r + DR2[d] / 2][c + DC2[d] / 2] = 1;
			CarvePath2(nr, nc);
		}
	}
}

static void CarveRoom(int rStart, int rEnd, int cStart, int cEnd)
{
	for (int r = rStart; r <= rEnd; ++r)
		for (int c = cStart; c <= cEnd; ++c)
			if (r >= 1 && r < GRID_SIZE2 - 1 && c >= 1 && c < GRID_SIZE2 - 1)
				sMaze2[r][c] = 1;
}

void Map2Scene::Initialize()
{
	Cube::LoadSharedMesh(Framework::GetDevice());
	Enemy::LoadSharedMesh(Framework::GetDevice());
	Item_ATK::LoadSharedMesh(Framework::GetDevice());
	Item_HP::LoadSharedMesh(Framework::GetDevice());
	Item_SPEED::LoadSharedMesh(Framework::GetDevice());

	// Map1(seed=0)과 다른 시드 사용
	srand(42);
	memset(sMaze2, 0, sizeof(sMaze2));
	CarvePath2(1, 1);

	// 입구 / 출구 외벽 뚫기
	sMaze2[0][1] = 1;
	sMaze2[GRID_SIZE2 - 1][GRID_SIZE2 - 2] = 1;

	// 오픈 룸 4개 추가 (Map1과 다른 구조적 특징)
	CarveRoom(3,  7,  3,  7);   // 1
	CarveRoom(3,  7,  17, 21);  // 2
	CarveRoom(3,  7,  33, 37);  // 3
	CarveRoom(17, 21, 3,  7);   // 4
	CarveRoom(17, 21, 33, 37);  // 5
	CarveRoom(19, 23, 17, 21);  // 6 (center)
	CarveRoom(33, 37, 3,  7);   // 7
	CarveRoom(33, 37, 33, 37);  // 8

	std::default_random_engine dre(std::random_device{}());
	std::uniform_int_distribution<int> height_uid(1, 10);
	std::uniform_int_distribution<int> enemy_uid(1, 100);

	for (int row = 0; row < GRID_SIZE2; ++row)
	{
		for (int col = 0; col < GRID_SIZE2; ++col)
		{
			Cube* cube = new Cube();
			cube->Initialize(Framework::GetDevice());

			float x = col * SPACING2 - OFFSET2;
			float z = row * SPACING2 - OFFSET2;

			bool isWall = (sMaze2[row][col] == 0);

			if (isWall)
			{
				cube->SetPosition(x, 0.0f, z);
				cube->SetScale(1.0f, 4.0f, 1.0f);
				// Map1(진회색)과 달리 어두운 파란색 벽
				cube->SetColor({ 0.1f, 0.15f, 0.4f, 1.0f });
			}
			else // 바닥
			{
				cube->SetPosition(x, -2.0f, z);
				cube->SetScale(1.0f, 0.1f * height_uid(dre), 1.0f);
				// Map1(연회색)과 달리 모래색 바닥
				cube->SetColor({ 0.85f, 0.65f, 0.35f, 1.0f });

				int dr = row - 1, dc = col - 1;
				if ((dr * dr + dc * dc) >= 25 && enemy_uid(dre) <= 5) {
					Enemy* enemy = new Enemy();
					enemy->Initialize(Framework::GetDevice());
					enemy->SetPosition(x, 0.0f, z);
					enemy->SetScale(0.04f, 0.04f, 0.04f);
					mEnemies.push_back(enemy);
					mGameObjects.push_back(enemy);
				}
			}

			mGameObjects.push_back(cube);
			mWallCubes.push_back(cube);
		}
	}

	mCubeAABBs.reserve(mWallCubes.size());
	for (auto* cube : mWallCubes) {
		cube->Update(0.0f);
		mCubeAABBs.push_back(cube->GetWorldAABB());
	}

	mPlayer = new Player();
	mPlayer->Initialize(Framework::GetDevice());
	// 41x41 그리드, offset=40 → row=1,col=1 → 월드(-38, 0, -38)
	mPlayer->SetPosition(-38.0f, 0.0f, -38.0f);
	mPlayer->SetColliders(&mCubeAABBs);
	mGameObjects.push_back(mPlayer);

	mGun = new Gun();
	mGun->Initialize(Framework::GetDevice());
	mGun->AttachTo(mPlayer);
	mGameObjects.push_back(mGun);

	mCrosshair = new Crosshair();
	mCrosshair->Initialize(Framework::GetDevice());

	UpdateFlowField();
	for (auto* e : mEnemies) {
		e->SetTarget(mPlayer->GetPositionPtr());
		e->SetColliders(&mCubeAABBs);
		e->SetFlowField(&mFlowField[0][0], kMazeSize, kMazeSpacing, kMazeOffset);
		e->SetEnemyList(&mEnemies);
	}

	for (int i = 0; i < kBulletPoolSize; ++i) {
		Bullet* b = new Bullet();
		b->Initialize(Framework::GetDevice());
		b->SetColliders(&mCubeAABBs);
		mBullets.push_back(b);
	}

	SpawnItems();
	ShowCursor(FALSE);
}

void Map2Scene::SpawnItems()
{
	std::vector<std::pair<float, float>> floors;
	for (int row = 0; row < GRID_SIZE2; ++row) {
		for (int col = 0; col < GRID_SIZE2; ++col) {
			if (sMaze2[row][col] == 0) continue;
			int dr = row - 1, dc = col - 1;
			if ((dr * dr + dc * dc) < 25) continue;
			floors.push_back({ col * SPACING2 - OFFSET2, row * SPACING2 - OFFSET2 });
		}
	}

	std::shuffle(floors.begin(), floors.end(), std::default_random_engine{ std::random_device{}() });

	static constexpr int kPerType = 10;
	if ((int)floors.size() < kPerType * 3) return;

	int idx = 0;
	auto spawn = [&](Item* item) {
		item->Initialize(Framework::GetDevice());
		item->SetPosition(floors[idx].first, 0.5f, floors[idx].second);
		mItems.push_back(item);
		++idx;
	};

	for (int i = 0; i < kPerType; ++i) spawn(new Item_ATK());
	for (int i = 0; i < kPerType; ++i) spawn(new Item_HP());
	for (int i = 0; i < kPerType; ++i) spawn(new Item_SPEED());
}

void Map2Scene::CheckItemPickup()
{
	XMFLOAT3 pPos = mPlayer->GetPosition();
	for (auto* item : mItems) {
		if (!item->IsActive()) continue;
		XMFLOAT3 iPos = item->GetPosition();
		float dx = pPos.x - iPos.x;
		float dz = pPos.z - iPos.z;
		if (dx * dx + dz * dz < 2.0f * 2.0f)
			item->OnPickup(mPlayer);
	}
}

void Map2Scene::UpdateFlowField()
{
	XMFLOAT3 pPos = mPlayer->GetPosition();
	int pRow = std::clamp((int)roundf((pPos.z + kMazeOffset) / kMazeSpacing), 0, kMazeSize - 1);
	int pCol = std::clamp((int)roundf((pPos.x + kMazeOffset) / kMazeSpacing), 0, kMazeSize - 1);

	memset(mFlowField, -1, sizeof(mFlowField));

	static std::pair<int, int> buf[kMazeSize * kMazeSize];
	int head = 0, tail = 0;

	if (sMaze2[pRow][pCol]) {
		mFlowField[pRow][pCol] = 0;
		buf[tail++] = { pRow, pCol };
	}

	static const int DR[] = { 0, 0, -1, 1 };
	static const int DC[] = { -1, 1,  0, 0 };

	while (head < tail) {
		auto [r, c] = buf[head++];
		for (int d = 0; d < 4; d++) {
			int nr = r + DR[d], nc = c + DC[d];
			if (nr < 0 || nr >= kMazeSize || nc < 0 || nc >= kMazeSize) continue;
			if (sMaze2[nr][nc] == 0 || mFlowField[nr][nc] != -1) continue;
			mFlowField[nr][nc] = mFlowField[r][c] + 1;
			buf[tail++] = { nr, nc };
		}
	}
}

void Map2Scene::Update(float dt)
{
	UpdateFlowField();

	for (const auto& obj : mGameObjects)
		obj->Update(dt);

	if (!debugMode && Input::GetKeyDown(eKeyCode::LButton))
		FireBullet();

	for (auto* b : mBullets)
		b->Update(dt);

	CheckBulletEnemyCollision();
	CheckEnemyPlayerCollision();

	for (auto* item : mItems)
		item->Update(dt);

	CheckItemPickup();
}

void Map2Scene::FireBullet()
{
	for (auto* b : mBullets) {
		if (!b->IsActive()) {
			b->Fire(mGun->GetMuzzlePosition(), mPlayer->GetLookDir(), mPlayer->GetATK());
			break;
		}
	}
}

void Map2Scene::CheckEnemyPlayerCollision()
{
	DirectX::BoundingBox playerAABB = mPlayer->GetWorldAABB();
	for (auto* e : mEnemies) {
		if (!e->IsAlive()) continue;
		if (e->GetWorldAABB().Intersects(playerAABB))
			mPlayer->TakeDamage(10);
	}
}

void Map2Scene::CheckBulletEnemyCollision()
{
	for (auto* b : mBullets) {
		if (!b->IsActive()) continue;
		DirectX::BoundingBox bAABB = b->GetWorldAABB();
		for (auto* e : mEnemies) {
			if (!e->IsAlive()) continue;
			if (bAABB.Intersects(e->GetWorldAABB())) {
				e->TakeDamage(b->GetDamage());
				b->Deactivate();
				break;
			}
		}
	}
}

void Map2Scene::Render(ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	XMMATRIX view = XMMatrixLookToLH(
		XMLoadFloat3(&Camera::camPos),
		XMVectorSet(Camera::camForward.x, Camera::camForward.y, Camera::camForward.z, 0),
		XMVectorSet(0, 1, 0, 0)
	);
	XMMATRIX proj = XMMatrixPerspectiveFovLH(70.0f * XM_PI / 180.0f, 1280.0f / 720.0f, 0.1f, 1000.0f);

	for (const auto& obj : mGameObjects)
		obj->Render(commandList, view, proj);

	for (auto* b : mBullets)
		b->Render(commandList, view, proj);

	for (auto* item : mItems)
		if (item->IsActive())
			item->Render(commandList, view, proj);

	mCrosshair->Render(commandList, view, proj);
}

void Map2Scene::Release()
{
	for (auto obj : mGameObjects)
		delete obj;
	mGameObjects.clear();
	mWallCubes.clear();
	mEnemies.clear();
	mCubeAABBs.clear();
	for (auto* b : mBullets) delete b;
	mBullets.clear();
	for (auto* item : mItems) delete item;
	mItems.clear();
	delete mCrosshair;
	mCrosshair = nullptr;
	Cube::UnloadSharedMesh();
	Enemy::UnloadSharedMesh();
	Item_ATK::UnloadSharedMesh();
	Item_HP::UnloadSharedMesh();
	Item_SPEED::UnloadSharedMesh();
	ShowCursor(TRUE);
}
