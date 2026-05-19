#include "Map1Scene.h"
#include "Cube.h"
#include "framework.h"
#include <random>
#include "Camera.h"
#include "Player.h"
#include "Bullet.h"


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
			else
			{
				cube->SetPosition(x, -1.5f, z);
				cube->SetScale(1.0f, 0.1f * uid(dre), 1.0f);
				cube->SetColor({ 0.8f, 0.8f, 0.8f, 1.0f });
			}

			mGameObjects.push_back(cube);
			mWallCubes.push_back(cube);
		}
	}

	mPlayer = new Player();
	mPlayer->Initialize(Framework::GetDevice());
	mPlayer->SetPosition(0.0f, 5.0f, -0.0f);
	mGameObjects.push_back(mPlayer);

	mGun = new Gun();
	mGun->Initialize(Framework::GetDevice());
	mGun->SetPosition(2.0f, 5.0f, -0.0f);
	mGameObjects.push_back(mGun);

	Enemy* enemy = new Enemy();
	enemy->Initialize(Framework::GetDevice());
	enemy->SetPosition(5.0f, 5.0f, 0.0f);
	enemy->SetScale(0.04f, 0.04f, 0.04f);
	mEnemies.push_back(enemy);
	mGameObjects.push_back(enemy);

	Bullet* bullet = new Bullet();
	bullet->Initialize(Framework::GetDevice());
	bullet->SetPosition(0.0f, 5.0f, 2.0f);
	
	mGameObjects.push_back(bullet);

}

void Map1Scene::Update(float dt)
{
	for (const auto& obj : mGameObjects)
		obj->Update(dt);
}

void Map1Scene::Render(ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	XMMATRIX view = XMMatrixLookToLH(
		XMLoadFloat3(&Camera::camPos),
		XMVectorSet(Camera::camForward.x, Camera::camForward.y, Camera::camForward.z, 0),
		XMVectorSet(0, 1, 0, 0)
	);
	XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 1280.0f / 720.0f, 0.1f, 1000.0f);

	for (const auto& obj : mGameObjects)
		obj->Render(commandList, view, proj);

	ImGui::Begin("Map1");
	ImGui::Text("Camera: (%.2f, %.2f, %.2f)", Camera::camPos.x, Camera::camPos.y, Camera::camPos.z);
	ImGui::Text("Total: %d cubes", (int)mGameObjects.size());
	ImGui::End();
}

void Map1Scene::Release()
{
	for (auto obj : mGameObjects)
		delete obj;
	mGameObjects.clear();
	mWallCubes.clear();
	mEnemies.clear();
	Cube::UnloadSharedMesh();
	Enemy::UnloadSharedMesh();
}