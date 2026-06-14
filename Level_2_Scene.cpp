#include "stdafx.h"
#include "Level_2_Scene.h"
#include "SceneManager.h"
#include "Camera.h"
#include "Input.h"
#include "framework.h"
#include "PickingUtils.h"

extern bool debugMode;

void Level_2_Scene::Initialize()
{
	debugMode = false;

	auto device = Framework::GetDevice();

	mTerrain = std::make_unique<Terrain>();
	mTerrain->Initialize(device);

	size_t numTanks = 15;

	mTankBodyRenderer = std::make_unique<TankBody>();
	mTankLidRenderer = std::make_unique<TankLid>();
	mTankBarrelRenderer = std::make_unique<TankBarrel>();

	mTankBodyRenderer->Initialize(device, numTanks);
	mTankLidRenderer->Initialize(device, numTanks);
	mTankBarrelRenderer->Initialize(device, numTanks);

	std::random_device rd;
	std::mt19937 gen(rd());

	std::uniform_real_distribution<float> distX(-100.f, 100.f);
	std::uniform_real_distribution<float> distZ(40.f, 250.f);

	mTanks.reserve(numTanks);

	for (size_t i = 0; i < numTanks; ++i)
	{
		auto tank = std::make_unique<Tank>();
		float randX = distX(gen);
		float randZ = distZ(gen);
		float groundY = mTerrain->GetHeightAt(randX, randZ);
		tank->SetPosition(randX, groundY, randZ);
		mTanks.emplace_back(std::move(tank));
	}

	mPlayerBodyRenderer   = std::make_unique<TankBody>();
	mPlayerLidRenderer    = std::make_unique<TankLid>();
	mPlayerBarrelRenderer = std::make_unique<TankBarrel>();
	mPlayerBodyRenderer->Initialize(device, 1);
	mPlayerLidRenderer->Initialize(device, 1);
	mPlayerBarrelRenderer->Initialize(device, 1);

	mPlayerBodyRenderer->SetColor(0.53f, 0.81f, 0.98f);
	mPlayerLidRenderer->SetColor(0.53f, 0.81f, 0.98f);
	mPlayerBarrelRenderer->SetColor(0.45f, 0.72f, 0.90f);

	mPlayerTank = std::make_unique<Tank>();
	float spawnY = mTerrain->GetHeightAt(0.f, 0.f);
	mPlayerTank->SetPosition(0.f, spawnY, 0.f);

	for (int i = 0; i < kMissilePoolSize; ++i)
	{
		mMissilePool[i] = new Missile();
		mMissilePool[i]->Initialize(device);
	}

	std::uniform_real_distribution<float> distScale(2.f, 6.f);
	std::uniform_real_distribution<float> distRotY(0.f, 360.f);

	for (int i = 0; i < 50; ++i)
	{
		float cx = distX(gen);
		float cz = distZ(gen);
		float cy = mTerrain->GetHeightAt(cx, cz);

		auto* cube = new Cube();
		cube->Initialize(device);
		float sx = distScale(gen);
		float sy = distScale(gen);
		float sz = distScale(gen);
		cube->SetScale(sx, sy, sz);
		cube->SetPosition(cx, cy, cz);
		cube->SetRotation(0.f, distRotY(gen), 0.f);
		cube->Update(0.f);
		mGameObjects.push_back(cube);
	}

	mExplosion = std::make_unique<ExplosionSystem>();
	mExplosion->Initialize(device);

	mShield = std::make_unique<GameObject>();
	mShield->Initialize(device);
	mShield->LoadFromOBJ("sphere.obj", device);
	mShield->SetScale(10.f, 10.f, 10.f);
	mShield->SetColor(0.4f, 0.8f, 1.f);

	mYouWin = std::make_unique<LetterObj>();
	mYouWin->Initialize(device, "YOUWIN.OBJ");
}

void Level_2_Scene::Update(float dt)
{
	if (Input::GetKeyDown(eKeyCode::ESC))
		SceneManager::LoadScene(L"MenuScene");

	mShieldActive = Input::GetKey(eKeyCode::X);

	if (!mWinActive)
	{
		bool allDead = std::all_of(mTanks.begin(), mTanks.end(),
			[](const std::unique_ptr<Tank>& t) { return !t->IsAlive(); });
		if (allDead || Input::GetKeyDown(eKeyCode::E))
			mWinActive = true;
	}

	if (mWinActive)
	{
		mWinTimer += dt;
		if (mWinTimer >= 3.f)
		{
			SceneManager::LoadScene(L"Level_3");
			return;
		}

		float yaw = atan2f(-Camera::camForward.z, -Camera::camForward.x);

		XMVECTOR camPos = XMLoadFloat3(&Camera::camPos);
		XMVECTOR camFwd = XMLoadFloat3(&Camera::camForward);
		XMVECTOR camUp  = XMLoadFloat3(&Camera::camUp);
		XMVECTOR textPos = camPos + camFwd * 8.f + camUp * 1.5f;
		XMFLOAT3 textPosF;
		XMStoreFloat3(&textPosF, textPos);

		mYouWin->SetPosition(textPosF);
		mYouWin->SetRotation(0.f, -XMConvertToDegrees(yaw) - 90.f, 0.f);
		mYouWin->Update(0.f);
	}

	if (Input::GetKeyDown(eKeyCode::RButton))
	{
		float screenW = (float)Framework::GetWidth();
		float screenH = (float)Framework::GetHeight();

		POINT pt;
		GetCursorPos(&pt);
		ScreenToClient(Framework::GetHwnd(), &pt);

		float ndcX = (2.f * pt.x / screenW) - 1.f;
		float ndcY = 1.f - (2.f * pt.y / screenH);

		XMMATRIX pickView = XMMatrixLookToLH(
			XMLoadFloat3(&Camera::camPos),
			XMVectorSet(Camera::camForward.x, Camera::camForward.y, Camera::camForward.z, 0.f),
			XMVectorSet(Camera::camUp.x, Camera::camUp.y, Camera::camUp.z, 0.f));
		XMMATRIX pickProj = XMMatrixPerspectiveFovLH(XM_PIDIV4, screenW / screenH, 0.1f, 1000.f);

		XMVECTOR rayViewDir = XMVector3TransformCoord(
			XMVectorSet(ndcX, ndcY, 1.f, 0.f), XMMatrixInverse(nullptr, pickProj));
		XMVECTOR rayDir = XMVector3Normalize(
			XMVector3TransformNormal(XMVectorSetW(rayViewDir, 0.f), XMMatrixInverse(nullptr, pickView)));
		XMVECTOR rayOrigin = XMLoadFloat3(&Camera::camPos);

		mSelectedTank = -1;
		float nearest = FLT_MAX;
		for (int i = 0; i < (int)mTanks.size(); ++i)
		{
			if (!mTanks[i]->IsAlive()) continue;
			float dist;
			if (mTanks[i]->GetWorldOBB().Intersects(rayOrigin, rayDir, dist) && dist < nearest)
			{
				nearest = dist;
				mSelectedTank = i;
			}
		}
	}

	if (Input::GetKeyDown(eKeyCode::Q)
		&& mSelectedTank >= 0 && mSelectedTank < (int)mTanks.size()
		&& mTanks[mSelectedTank]->IsAlive())
	{
		XMFLOAT3 firePos = { mPlayerTank->mBarrelMatrix._41,
		                     mPlayerTank->mBarrelMatrix._42,
		                     mPlayerTank->mBarrelMatrix._43 };
		XMFLOAT3 targetPos = mTanks[mSelectedTank]->GetWorldOBB().Center;

		XMVECTOR dir = XMVector3Normalize(XMLoadFloat3(&targetPos) - XMLoadFloat3(&firePos));
		XMFLOAT3 dirF;
		XMStoreFloat3(&dirF, dir);

		for (int i = 0; i < kMissilePoolSize; ++i)
		{
			if (mMissilePool[i]->IsDead())
			{
				mMissilePool[i]->Spawn(firePos, dirF, 80.f);
				mMissilePool[i]->SetTarget(mTanks[mSelectedTank].get());
				break;
			}
		}
	}

	constexpr float kMoveSpeed = 15.0f;
	constexpr float kTurnSpeed = 1.5f;

	XMFLOAT3 pos = mPlayerTank->GetPosition();
	const XMFLOAT3 savedPos = pos;
	float sinH = sinf(mPlayerTank->mHeading);
	float cosH = cosf(mPlayerTank->mHeading);

	if (Input::GetKey(eKeyCode::W)) { pos.x -= sinH * kMoveSpeed * dt; pos.z -= cosH * kMoveSpeed * dt; }
	if (Input::GetKey(eKeyCode::S)) { pos.x += sinH * kMoveSpeed * dt; pos.z += cosH * kMoveSpeed * dt; }

	if (Input::GetKey(eKeyCode::A)) mPlayerTank->mHeading -= kTurnSpeed * dt;
	if (Input::GetKey(eKeyCode::D)) mPlayerTank->mHeading += kTurnSpeed * dt;

	pos.y = mTerrain->GetHeightAt(pos.x, pos.z);
	mPlayerTank->SetPosition(pos);

	if (Input::GetKey(eKeyCode::LButton))
	{
		POINT curr;
		GetCursorPos(&curr);
		if (!mMouseRotating) { mPrevMousePos = curr; mMouseRotating = true; }

		constexpr float kSensH = 0.3f;
		constexpr float kSensV = 0.3f;
		float dx = static_cast<float>(curr.x - mPrevMousePos.x);
		float dy = static_cast<float>(curr.y - mPrevMousePos.y);

		mPlayerTank->mLidRotation.y -= dx * kSensH;

		mPlayerTank->mBarrelRotation.x -= dy * kSensV;
		mPlayerTank->mBarrelRotation.x  = std::clamp(mPlayerTank->mBarrelRotation.x, -10.0f, 20.0f);

		mPrevMousePos = curr;
	}
	else { mMouseRotating = false; }

	{
		constexpr float kStep   = 1.0f;
		constexpr float kSmooth = 8.0f;

		float sh = sinf(mPlayerTank->mHeading);
		float ch = cosf(mPlayerTank->mHeading);

		float sFwd = (mTerrain->GetHeightAt(pos.x - sh * kStep, pos.z - ch * kStep)
		            - mTerrain->GetHeightAt(pos.x + sh * kStep, pos.z + ch * kStep))
		           / (2.0f * kStep);

		float sLat = (mTerrain->GetHeightAt(pos.x - ch * kStep, pos.z + sh * kStep)
		            - mTerrain->GetHeightAt(pos.x + ch * kStep, pos.z - sh * kStep))
		           / (2.0f * kStep);

		float targetPitch = atan2f(sFwd, 1.0f);
		float targetRoll  = atan2f(sLat, 1.0f);

		mPlayerTank->mPitch += (targetPitch - mPlayerTank->mPitch) * kSmooth * dt;
		mPlayerTank->mRoll  += (targetRoll  - mPlayerTank->mRoll)  * kSmooth * dt;
	}

	mPlayerTank->Update(dt);

	{
		DirectX::BoundingOrientedBox tankOBB = mPlayerTank->GetWorldOBB();
		for (auto* obj : mGameObjects)
		{
			if (tankOBB.Intersects(obj->GetWorldOBB()))
			{
				mPlayerTank->SetPosition(savedPos);
				mPlayerTank->Update(dt);
				break;
			}
		}
	}

	mPlayerBodyRenderer->SetWorldMatrix(mPlayerTank->mBodyMatrix);
	mPlayerLidRenderer->SetWorldMatrix(mPlayerTank->mLidMatrix);
	mPlayerBarrelRenderer->SetWorldMatrix(mPlayerTank->mBarrelMatrix);

	if (mShieldActive)
	{
		XMFLOAT3 center = mPlayerTank->GetWorldOBB().Center;
		mShield->SetPosition(center);
		mShield->Update(0.f);
	}

	constexpr float kCamDist   = 12.0f;
	constexpr float kCamHeight = 5.0f;

	float turretWorldYaw = mPlayerTank->mHeading + XMConvertToRadians(mPlayerTank->mLidRotation.y);
	float sinTY = sinf(turretWorldYaw);
	float cosTY = cosf(turretWorldYaw);

	XMVECTOR tankPosV = XMLoadFloat3(&pos);
	XMVECTOR worldUp  = XMVectorSet(0.f, 1.f, 0.f, 0.f);

	XMVECTOR camPosV  = tankPosV
		+ XMVectorSet(sinTY, 0.f, cosTY, 0.f) * kCamDist
		+ worldUp * kCamHeight;
	XMStoreFloat3(&Camera::camPos, camPosV);

	XMVECTOR lookTarget = tankPosV + worldUp * 1.0f;
	XMVECTOR fwdV   = XMVector3Normalize(lookTarget - camPosV);
	XMVECTOR rightV = XMVector3Normalize(XMVector3Cross(worldUp, fwdV));
	XMVECTOR upV    = XMVector3Cross(fwdV, rightV);
	XMStoreFloat3(&Camera::camForward, fwdV);
	XMStoreFloat3(&Camera::camRight, rightV);
	XMStoreFloat3(&Camera::camUp, upV);

	mBodyMats.clear(); mLidMats.clear(); mBarrelMats.clear();

	for (auto& tank : mTanks)
	{
		tank->mLidOffset      = mTanks[0]->mLidOffset;
		tank->mLidRotation    = mTanks[0]->mLidRotation;
		tank->mBarrelOffset   = mTanks[0]->mBarrelOffset;
		tank->mBarrelPivot    = mTanks[0]->mBarrelPivot;
		tank->mBarrelRotation = mTanks[0]->mBarrelRotation;
		tank->Update(dt);

		if (tank->IsAlive())
		{
			mBodyMats.push_back(tank->mBodyMatrix);
			mLidMats.push_back(tank->mLidMatrix);
			mBarrelMats.push_back(tank->mBarrelMatrix);
		}
	}

	mTankBodyRenderer->UpdateInstances(mBodyMats);
	mTankLidRenderer->UpdateInstances(mLidMats);
	mTankBarrelRenderer->UpdateInstances(mBarrelMats);

	for (int i = 0; i < kMissilePoolSize; ++i)
	{
		mMissilePool[i]->Update(dt);
		if (mMissilePool[i]->IsDead()) continue;

		XMVECTOR mpos = XMLoadFloat3(mMissilePool[i]->GetPositionPtr());
		for (auto& tank : mTanks)
		{
			if (!tank->IsAlive()) continue;
			if (tank->GetWorldOBB().Contains(mpos) != DirectX::DISJOINT)
			{
				XMFLOAT3 hitPos = tank->GetPosition();
				tank->Hit();
				mMissilePool[i]->Kill();
				if (!tank->IsAlive())
					mExplosion->Spawn(hitPos);
				break;
			}
		}

		if (!mMissilePool[i]->IsDead())
		{
			for (auto* obj : mGameObjects)
			{
				if (obj->GetWorldOBB().Contains(mpos) != DirectX::DISJOINT)
				{
					mMissilePool[i]->Kill();
					break;
				}
			}
		}
	}

	mExplosion->Update(dt);
}

void Level_2_Scene::Render(ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	XMMATRIX view = XMMatrixLookToLH(
		XMLoadFloat3(&Camera::camPos),
		XMVectorSet(Camera::camForward.x, Camera::camForward.y, Camera::camForward.z, 0),
		XMVectorSet(Camera::camUp.x, Camera::camUp.y, Camera::camUp.z, 0));
	XMMATRIX proj = XMMatrixPerspectiveFovLH(
		XM_PIDIV4, (float)Framework::GetWidth() / (float)Framework::GetHeight(),
		0.1f, 1000.0f);

	for (const auto& obj : mGameObjects)
		obj->Render(commandList, view, proj);

	mTerrain->Render(commandList, view, proj);

	mTankBodyRenderer->Render(commandList, view, proj);
	mTankLidRenderer->Render(commandList, view, proj);
	mTankBarrelRenderer->Render(commandList, view, proj);
	mTankBodyRenderer->RenderOBBs(commandList, view, proj);

	mPlayerBodyRenderer->Render(commandList, view, proj);
	mPlayerLidRenderer->Render(commandList, view, proj);
	mPlayerBarrelRenderer->Render(commandList, view, proj);

	for (int i = 0; i < kMissilePoolSize; ++i)
		mMissilePool[i]->Render(commandList, view, proj);

	mExplosion->Render(commandList, view, proj);

	if (mShieldActive)
		mShield->Render(commandList, view, proj);

	if (mWinActive)
		mYouWin->Render(commandList, view, proj);
}

void Level_2_Scene::Release()
{
	for (auto* obj : mGameObjects)
		delete obj;
	mGameObjects.clear();

	for (int i = 0; i < kMissilePoolSize; ++i)
	{
		delete mMissilePool[i];
		mMissilePool[i] = nullptr;
	}
}
