#include "TitleScene.h"
#include "Camera.h"
#include "stdafx.h"
#include "framework.h"
#include "SceneManager.h"
#include "PickingUtils.h"
#include <random>

TitleScene::TitleScene()
{
}

TitleScene::~TitleScene()
{
}

void TitleScene::Initialize()
{

	Camera::camPitch = 0.0f;
	Camera::SetPosition(0, 0, -30.0f);


	mTitleText1 = new LetterObj();
	mTitleText1->Initialize(Framework::GetDevice(), "SM_Title1.obj");
	mTitleText1->SetPosition(0, 5.0f, 0);	

	mTitleText2 = new LetterObj();
	mTitleText2->Initialize(Framework::GetDevice(), "SM_Title2.obj");

	mName = new LetterObj();
	mName->Initialize(Framework::GetDevice(), "SM_NAME.obj");
	mName->SetPosition(0, -5.0f, 0);

	mGameObjects.push_back(mTitleText1);
	mGameObjects.push_back(mTitleText2);
	mGameObjects.push_back(mName);
}

void TitleScene::Update(float dt)
{
	float rotSpeed = 1.0f;
	mTitleText1->SetRotation({ mTitleText1->GetRotation().x, mTitleText1->GetRotation().y + rotSpeed * dt, mTitleText1->GetRotation().z });
	mTitleText2->SetRotation({ mTitleText2->GetRotation().x, mTitleText2->GetRotation().y + rotSpeed * dt, mTitleText2->GetRotation().z });

	if (!mExploding && Input::GetKeyDown(eKeyCode::LButton))
	{
		if (PickAABB(mName->GetWorldAABB()))
			SpawnExplosion();
	}

	if (mExploding)
	{
		mExplodeTimer += dt;

		for (auto& ec : mExplosionCubes)
		{
			XMFLOAT3 pos = ec.cube->GetPosition();
			pos.x += ec.velocity.x * dt;
			pos.y += ec.velocity.y * dt;
			pos.z += ec.velocity.z * dt;
			ec.velocity.y -= 25.f * dt;
			ec.cube->SetPosition(pos);

			XMFLOAT3 rot = ec.cube->GetRotation();
			rot.x += ec.angularVel.x * dt;
			rot.y += ec.angularVel.y * dt;
			rot.z += ec.angularVel.z * dt;
			ec.cube->SetRotation(rot);

			ec.cube->Update(dt);
		}

		if (mExplodeTimer >= kExplodeDuration)
			SceneManager::LoadScene(L"MenuScene");
	}

	for(auto obj : mGameObjects)
	{
		obj->Update(dt);
	}
}

void TitleScene::Render(ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	XMMATRIX view = XMMatrixLookToLH(XMLoadFloat3(&Camera::camPos), XMVectorSet(Camera::camForward.x, Camera::camForward.y, Camera::camForward.z, 0), XMVectorSet(0, 1, 0, 0));
	XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, (float)Framework::GetWidth() / (float)Framework::GetHeight(), 0.1f, 100.0f);

	for (auto obj : mGameObjects)
	{
		if (mExploding && obj == mName) continue;
		obj->Render(commandList, view, proj);
	}

	for (auto& ec : mExplosionCubes)
		ec.cube->Render(commandList, view, proj);
}

void TitleScene::SpawnExplosion()
{
	DirectX::BoundingBox aabb = mName->GetWorldAABB();
	XMFLOAT3 center = aabb.Center;
	XMFLOAT3 ext    = aabb.Extents;

	std::mt19937 rng(std::random_device{}());
	std::uniform_real_distribution<float> randPos(-1.f, 1.f);
	std::uniform_real_distribution<float> randSpeed(8.f, 20.f);
	std::uniform_real_distribution<float> randSpin(-5.f, 5.f);

	constexpr int COUNT = 40;
	for (int i = 0; i < COUNT; i++)
	{
		Cube* cube = new Cube();
		cube->Initialize(Framework::GetDevice());
		cube->SetScale(0.3f, 0.3f, 0.3f);

		float px = center.x + randPos(rng) * ext.x;
		float py = center.y + randPos(rng) * ext.y;
		float pz = center.z + randPos(rng) * ext.z;
		cube->SetPosition(px, py, pz);

		float dx = px - center.x;
		float dy = py - center.y;
		float dz = pz - center.z;
		float len = sqrtf(dx * dx + dy * dy + dz * dz);
		if (len > 0.001f) { dx /= len; dy /= len; dz /= len; }
		else { dx = randPos(rng); dy = 1.f; dz = randPos(rng); }

		float speed = randSpeed(rng);
		ExplodeCube ec;
		ec.cube     = cube;
		ec.velocity = { dx * speed, dy * speed + 5.f, dz * speed };
		ec.angularVel = { randSpin(rng), randSpin(rng), randSpin(rng) };
		mExplosionCubes.push_back(ec);
	}

	mExploding    = true;
	mExplodeTimer = 0.f;
}

void TitleScene::Release()
{
	for (auto& ec : mExplosionCubes)
		delete ec.cube;
	mExplosionCubes.clear();

	for (auto obj : mGameObjects)
		delete obj;
	mGameObjects.clear();
	mTitleText1 = mTitleText2 = mName = nullptr;
	mExploding = false;
	mExplodeTimer = 0.f;
}
