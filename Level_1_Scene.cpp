#include "stdafx.h"
#include "framework.h"
#include "Level_1_Scene.h"
#include "Gizumo.h"
#include "Camera.h"
#include "Map.h"
#include "SceneManager.h"

extern bool debugMode;

Level_1_Scene::Level_1_Scene()
{
}

Level_1_Scene::~Level_1_Scene()
{
}

void Level_1_Scene::Initialize()
{
	debugMode = false;

	Camera::SetPosition(0.f, 5.f, -15.f);

	Input::LockCursor(Framework::GetHwnd());

	mHelicopter = std::make_unique<Helicopter>();
	mHelicopter->Initialize(Framework::GetDevice());

	static const XMFLOAT3 kTankPositions[15] = {
		{   0.f, 0.f,  40.f }, {  30.f, 0.f,  40.f }, { -30.f, 0.f,  40.f },
		{  60.f, 0.f,  80.f }, { -60.f, 0.f,  80.f }, {   0.f, 0.f,  80.f },
		{  90.f, 0.f, 120.f }, { -90.f, 0.f, 120.f }, {  30.f, 0.f, 120.f },
		{ -30.f, 0.f, 120.f }, {  60.f, 0.f, 160.f }, { -60.f, 0.f, 160.f },
		{   0.f, 0.f, 160.f }, {  90.f, 0.f, 200.f }, { -90.f, 0.f, 200.f },
	};
	mTanks.resize(15);
	for (int i = 0; i < 15; i++)
	{
		mTanks[i] = std::make_unique<Tank>();
		mTanks[i]->Initialize(Framework::GetDevice());
		mTanks[i]->SetPosition(kTankPositions[i]);
	}

	mMap = new Map();
	mMap->Initialize(Framework::GetDevice());
	RebuildMapInstances();
	mGameObjects.push_back(mMap);

	for (int i = 0; i < kMissilePoolSize; i++)
	{
		mMissilePool[i] = new Missile();
		mMissilePool[i]->Initialize(Framework::GetDevice());
	}
}

void Level_1_Scene::Update(float dt)
{
	if (Input::GetKeyDown(eKeyCode::ESC))
		SceneManager::LoadScene(L"MenuScene");

	if (Input::GetKeyDown(eKeyCode::V))
		mFirstPerson = !mFirstPerson;

	if (Input::GetKeyDown(eKeyCode::F5))
	{
		if (Input::IsCursorLocked())
			Input::UnlockCursor();
		else
			Input::LockCursor(Framework::GetHwnd());
	}

	mHelicopter->Update(dt);

	XMFLOAT3 heliPos = mHelicopter->GetPosition();
	float heading    = mHelicopter->GetHeading();

	if (mFirstPerson)
	{
		XMMATRIX R = XMMatrixRotationRollPitchYaw(
			mHelicopter->GetTiltPitch(), heading, mHelicopter->GetTiltRoll());
		XMVECTOR worldOffset = XMVector3TransformNormal(XMLoadFloat3(&mFpvOffset), R);
		XMStoreFloat3(&Camera::camPos, XMLoadFloat3(&heliPos) + worldOffset);

		XMVECTOR fwd = XMVector3Normalize(XMVector3TransformNormal(XMVectorSet(0.f, 0.f, 1.f, 0.f), R));
		XMVECTOR up  = XMVector3Normalize(XMVector3TransformNormal(XMVectorSet(0.f, 1.f, 0.f, 0.f), R));
		XMStoreFloat3(&Camera::camForward, fwd);
		XMStoreFloat3(&Camera::camUp, up);
	}
	else
	{
		const float camDist   = 15.f;
		const float camHeight =  5.f;
		XMFLOAT3 targetCamPos = {
			heliPos.x - sinf(heading) * camDist,
			heliPos.y + camHeight,
			heliPos.z - cosf(heading) * camDist
		};

		const float camSpeed = 8.f;
		float t = 1.f - expf(-camSpeed * dt);
		XMVECTOR camPos    = XMLoadFloat3(&Camera::camPos);
		XMVECTOR targetPos = XMLoadFloat3(&targetCamPos);
		XMStoreFloat3(&Camera::camPos, XMVectorLerp(camPos, targetPos, t));

		XMVECTOR lookDir = XMVector3Normalize(
			XMLoadFloat3(&heliPos) - XMLoadFloat3(&Camera::camPos));
		XMStoreFloat3(&Camera::camForward, lookDir);
		Camera::camUp = { 0.f, 1.f, 0.f };
	}

	for (auto& tank : mTanks)
	{
		tank->mLidOffset      = mTanks[0]->mLidOffset;
		tank->mLidRotation    = mTanks[0]->mLidRotation;
		tank->mBarrelOffset   = mTanks[0]->mBarrelOffset;
		tank->mBarrelPivot    = mTanks[0]->mBarrelPivot;
		tank->mBarrelRotation = mTanks[0]->mBarrelRotation;
		tank->Update(dt);
	}

	for (const auto& obj : mGameObjects)
		obj->Update(dt);

	if (Input::GetKeyDown(eKeyCode::LButton))
	{
		XMFLOAT3 pos1, pos2, dir;
		mHelicopter->GetFireData(pos1, pos2, dir);
		XMFLOAT3 spawnPos = mFireFromLeft ? pos1 : pos2;
		mFireFromLeft = !mFireFromLeft;

		for (int i = 0; i < kMissilePoolSize; i++)
		{
			if (mMissilePool[i]->IsDead())
			{
				mMissilePool[i]->Spawn(spawnPos, dir);
				break;
			}
		}
	}

	for (int i = 0; i < kMissilePoolSize; i++)
		mMissilePool[i]->Update(dt);
}

void Level_1_Scene::Render(ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	XMMATRIX view = XMMatrixLookToLH(XMLoadFloat3(&Camera::camPos), XMVectorSet(Camera::camForward.x, Camera::camForward.y, Camera::camForward.z, 0), XMVectorSet(Camera::camUp.x, Camera::camUp.y, Camera::camUp.z, 0));
	XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 1280.0f / 720.0f, 0.1f, 1000.0f);

	BoundingFrustum frustum(proj);
	BoundingFrustum worldFrustum;
	frustum.Transform(worldFrustum, XMMatrixInverse(nullptr, view));

	ApplyFrustumCulling(worldFrustum);

	for (const auto& obj : mGameObjects)
	{
		if (obj != mMap)
		{
			if (!worldFrustum.Intersects(obj->GetWorldAABB()))
				continue;
		}
		obj->Render(commandList, view, proj);
	}

	mHelicopter->Render(commandList, view, proj);

	for (auto& tank : mTanks)
		tank->Render(commandList, view, proj);

	for (int i = 0; i < kMissilePoolSize; i++)
		mMissilePool[i]->Render(commandList, view, proj);

	ImGui::Begin("Settings");
	ImGui::Text("Camera: %s (V to toggle)", mFirstPerson ? "1인칭" : "3인칭");
	ImGui::Text("Position: (%.1f, %.1f, %.1f)", Camera::camPos.x, Camera::camPos.y, Camera::camPos.z);
	if (mFirstPerson)
		ImGui::DragFloat3("FPV Offset", &mFpvOffset.x, 0.01f);
	ImGui::Separator();
	ImGui::Text("Missile Offsets");
	ImGui::DragFloat3("Offset 1", &mHelicopter->mMissileOffset1.x, 0.05f);
	ImGui::DragFloat3("Offset 2", &mHelicopter->mMissileOffset2.x, 0.05f);
	ImGui::Separator();
	ImGui::Text("Tank Parts (Body 기준)");
	ImGui::DragFloat3("Lid Offset",      &mTanks[0]->mLidOffset.x,      1.0f);
	ImGui::DragFloat3("Lid Rotation",    &mTanks[0]->mLidRotation.x,    1.0f);
	ImGui::DragFloat3("Barrel Offset",   &mTanks[0]->mBarrelOffset.x,   1.0f);
	ImGui::DragFloat3("Barrel Pivot",    &mTanks[0]->mBarrelPivot.x,    1.0f);
	ImGui::DragFloat3("Barrel Rotation", &mTanks[0]->mBarrelRotation.x, 1.0f);
	ImGui::End();
}

void Level_1_Scene::RebuildMapInstances()
{
	constexpr int GRID = 5;
	float offsetX = (GRID - 1) * mMapSpacingX * 0.5f;
	float offsetZ = (GRID - 1) * mMapSpacingZ * 0.5f;

	mMap->ClearInstances();
	mAllTileMatrices.clear();
	for (int z = 0; z < GRID; z++)
		for (int x = 0; x < GRID; x++)
		{
			XMFLOAT3 pos = { x * mMapSpacingX - offsetX, 0.f, z * mMapSpacingZ - offsetZ };
			mMap->AddInstance(pos);

			XMFLOAT4X4 w;
			XMStoreFloat4x4(&w, XMMatrixTranslation(pos.x, pos.y, pos.z));
			mAllTileMatrices.push_back(w);
		}

	mMap->BuildInstanceBuffer(Framework::GetDevice());
}

void Level_1_Scene::ApplyFrustumCulling(const DirectX::BoundingFrustum& worldFrustum)
{
	float radius = sqrtf(mMapSpacingX * mMapSpacingX + mMapSpacingZ * mMapSpacingZ) * 0.5f;

	std::vector<XMFLOAT4X4> visible;
	visible.reserve(mAllTileMatrices.size());
	for (const auto& mat : mAllTileMatrices)
	{
		BoundingSphere sphere({ mat._41, mat._42, mat._43 }, radius);
		if (worldFrustum.Intersects(sphere))
			visible.push_back(mat);
	}

	mVisibleTileCount = (int)visible.size();
	mMap->UpdateInstancesForCulling(visible);
}

void Level_1_Scene::Release()
{
	Input::UnlockCursor();
	ClipCursor(nullptr);
	debugMode = true;
	for (auto obj : mGameObjects)
		delete obj;
	mGameObjects.clear();
	mMap = nullptr;
	mHelicopter.reset();
	mTanks.clear();
	mAllTileMatrices.clear();

	for (int i = 0; i < kMissilePoolSize; i++)
	{
		delete mMissilePool[i];
		mMissilePool[i] = nullptr;
	}
}
