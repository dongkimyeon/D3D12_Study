#include "Map1Scene.h"
#include "Cube.h"
#include "framework.h"
#include <random>
#include <algorithm>
#include "Camera.h"
#include "Player.h"
#include "Bullet.h"
#include "Crosshair.h"

extern bool debugMode;


Map1Scene::Map1Scene() {}
Map1Scene::~Map1Scene() {}

static const int GRID_SIZE = 51;
static const float SPACING = 2.0f;
static const float OFFSET = (GRID_SIZE - 1) * SPACING * 0.5f;

static int sMaze[GRID_SIZE][GRID_SIZE];

static const int DR[] = { 0,  0, -2,  2 };
static const int DC[] = { -2,  2,  0,  0 };

static void Shuffle(int* arr, int n)
{
	for (int i = n - 1; i > 0; --i)
	{
		int j = rand() % (i + 1);
		int tmp = arr[i];
		arr[i] = arr[j];
		arr[j] = tmp;
	}
}

static void CarvePath(int r, int c)
{
	sMaze[r][c] = 1;
	int order[4] = { 0, 1, 2, 3 };
	Shuffle(order, 4);
	for (int i = 0; i < 4; ++i)
	{
		int d = order[i];
		int nr = r + DR[d];
		int nc = c + DC[d];
		if (nr >= 1 && nr < GRID_SIZE - 1 &&
			nc >= 1 && nc < GRID_SIZE - 1 &&
			sMaze[nr][nc] == 0)
		{
			sMaze[r + DR[d] / 2][c + DC[d] / 2] = 1;
			CarvePath(nr, nc);
		}
	}
}

void Map1Scene::Initialize()
{
	Cube::LoadSharedMesh(Framework::GetDevice());
	Enemy::LoadSharedMesh(Framework::GetDevice());

	srand(0);
	memset(sMaze, 0, sizeof(sMaze));
	CarvePath(1, 1);

	// 입구 / 출구 외벽 뚫기
	sMaze[0][1] = 1;
	sMaze[GRID_SIZE - 1][GRID_SIZE - 2] = 1;

	std::default_random_engine dre(std::random_device{}());
	std::uniform_int_distribution<int> uid(1, 10);

	for (int row = 0; row < GRID_SIZE; ++row)
	{
		for (int col = 0; col < GRID_SIZE; ++col)
		{
			Cube* cube = new Cube();
			cube->Initialize(Framework::GetDevice());

			float x = col * SPACING - OFFSET;
			float z = row * SPACING - OFFSET;

			bool isWall = (sMaze[row][col] == 0);

			if (isWall)
			{
				cube->SetPosition(x, 0.0f, z);
				cube->SetScale(1.0f, 3.0f, 1.0f);
				cube->SetColor({ 0.2f, 0.2f, 0.2f, 1.0f });
			}
			else //바닥
			{
				cube->SetPosition(x, -1.5f, z);
				cube->SetScale(1.0f, 0.1f * uid(dre), 1.0f);
				cube->SetColor({ 0.8f, 0.8f, 0.8f, 1.0f });

				// 플레이어 스폰(row=1,col=1) 반경 5칸 이내 제외, 20% 확률로 적 생성
				int dr = row - 1, dc = col - 1;
				if ((dr * dr + dc * dc) >= 25 && uid(dre) <= 2) {
					Enemy* enemy = new Enemy();
					enemy->Initialize(Framework::GetDevice());
					enemy->SetPosition(x, 0.5f, z);
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
	Camera::SetColliders(&mCubeAABBs);

	// 큐브 인스턴스 버퍼 1회 빌드 (큐브는 정적 → 매 프레임 갱신 불필요)
	Cube::BuildInstanceBuffer(Framework::GetDevice(), mWallCubes);

	// 큐브 인스턴스 버퍼 1회 빌드 (큐브는 정적 오브젝트)
	Cube::BuildInstanceBuffer(Framework::GetDevice(), mWallCubes);

	mPlayer = new Player();
	mPlayer->Initialize(Framework::GetDevice());
	// 입구: row=0,col=1 → 월드(-48, z=-50). 바로 안쪽 z=-48에 배치
	mPlayer->SetPosition(-48.0f, 2.0f, -48.0f);
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

	// 적 인스턴스 버퍼 용량 할당
	Enemy::BuildInstanceBuffer(Framework::GetDevice(), (UINT)mEnemies.size());

	ShowCursor(FALSE);
}

void Map1Scene::UpdateFlowField()
{
	XMFLOAT3 pPos = mPlayer->GetPosition();
	int pRow = std::clamp((int)roundf((pPos.z + kMazeOffset) / kMazeSpacing), 0, kMazeSize - 1);
	int pCol = std::clamp((int)roundf((pPos.x + kMazeOffset) / kMazeSpacing), 0, kMazeSize - 1);

	memset(mFlowField, -1, sizeof(mFlowField));

	// 정적 배열 BFS (힙 할당 없음)
	static std::pair<int, int> buf[kMazeSize * kMazeSize];
	int head = 0, tail = 0;

	if (sMaze[pRow][pCol]) {
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
			if (sMaze[nr][nc] == 0 || mFlowField[nr][nc] != -1) continue;
			mFlowField[nr][nc] = mFlowField[r][c] + 1;
			buf[tail++] = { nr, nc };
		}
	}
}

void Map1Scene::Update(float dt)
{
	UpdateFlowField();

	for (const auto& obj : mGameObjects)
		obj->Update(dt);

	if (mPlayer->GetPosition().y < -15.0f)
		mPlayer->Respawn({ -48.0f, 2.0f, -48.0f });

	if (!debugMode && Input::GetKeyDown(eKeyCode::LButton))
		FireBullet();

	for (auto* b : mBullets)
		b->Update(dt);

	CheckBulletEnemyCollision();
	CheckEnemyPlayerCollision();
}

void Map1Scene::FireBullet()
{
	for (auto* b : mBullets) {
		if (!b->IsActive()) {
			b->Fire(mGun->GetMuzzlePosition(), mPlayer->GetLookDir(), mPlayer->GetATK());
			break;
		}
	}
}

void Map1Scene::CheckEnemyPlayerCollision()
{
	DirectX::BoundingBox playerAABB = mPlayer->GetWorldAABB();
	for (auto* e : mEnemies) {
		if (!e->IsAlive()) continue;
		if (e->GetWorldAABB().Intersects(playerAABB))
			mPlayer->TakeDamage(10);
	}
}

void Map1Scene::CheckBulletEnemyCollision()
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

void Map1Scene::Render(ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	XMMATRIX view = XMMatrixLookToLH(
		XMLoadFloat3(&Camera::camPos),
		XMVectorSet(Camera::camForward.x, Camera::camForward.y, Camera::camForward.z, 0),
		XMVectorSet(0, 1, 0, 0)
	);
	XMMATRIX proj = XMMatrixPerspectiveFovLH(70.0f * XM_PI / 180.0f, 1280.0f / 720.0f, 0.1f, 1000.0f);

	// viewProj를 프레임당 1회 루트 상수로 설정
	XMFLOAT4X4 vpT;
	XMStoreFloat4x4(&vpT, XMMatrixTranspose(view * proj));
	commandList->SetGraphicsRoot32BitConstants(0, 16, &vpT.m[0][0], 0);

	// 비-배치 오브젝트 (Player, Gun, Bullet 등)
	for (const auto& obj : mGameObjects)
	{
		Player* playerPtr = dynamic_cast<Player*>(obj);
		if(playerPtr && Camera::sMode == eCameraMode::FirstPerson)
			continue;
		obj->Render(commandList, view, proj);
	}

	for (auto* b : mBullets)
		b->Render(commandList, view, proj);

	// 배치 렌더 (인스턴싱)
	Cube::RenderBatch(commandList);
	Enemy::RenderBatch(commandList, mEnemies);

	if (Camera::sMode == eCameraMode::FirstPerson)
		mCrosshair->Render(commandList, view, proj);
}

void Map1Scene::Release()
{
	Camera::SetColliders(nullptr);
	for (auto obj : mGameObjects)
		delete obj;
	mGameObjects.clear();
	mWallCubes.clear();
	mEnemies.clear();
	mCubeAABBs.clear();
	for (auto* b : mBullets) delete b;
	mBullets.clear();
	delete mCrosshair;
	mCrosshair = nullptr;
	Cube::UnloadSharedMesh();
	Enemy::UnloadSharedMesh();
	ShowCursor(TRUE);
}